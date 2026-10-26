#include <Content/HLSL/SCMap/SCMapRender.hlsli>

// OpenBW ui.h draw_tiles의 dirs 순서 (비트 i = kCreepNeighborOffsets[i] 이웃이 Creep)
// 0 오른쪽아래, 1 아래, 2 왼쪽아래, 3 오른쪽, 4 왼쪽, 5 오른쪽위, 6 위, 7 왼쪽위  (+x 오른쪽, +y 아래)
static const int2 kCreepNeighborOffsets[8] =
{
	int2(1, 1), int2(0, 1), int2(-1, 1),
	int2(1, 0), int2(-1, 0),
	int2(1, -1), int2(0, -1), int2(-1, -1),
};

static const uint kMinitilesPerMegatileAxis = 4; // 메가타일 = 4x4 미니타일
static const uint kMinitilePixelSize = 8; // 미니타일 = 8x8 픽셀

// MTXM / 크립 상태로부터 이 타일의 메가타일 번호(VX4 색인)를 구한다
// cv5_count: CV5 원소 수 (= 그룹 수 * kCV5MegatilesPerGroup, g_cv5.GetDimensions 결과)
uint ResolveMegatile(int2 megatile_pos, int2 map_size_megatile, uint cv5_count, bool has_creep)
{
	uint cv5_idx = 0;

	if (has_creep == false)
	{
		// CV5는 그룹 배열이고, 그룹마다 메가타일 번호 16개를 가진다
		// MTXM 값(OpenBW tile_id) = cv5_group_idx << 4 | cv5_sub_idx
		//   cv5_group_idx = (mtxm >> 4) & 0x7FF
		//   cv5_sub_idx   = mtxm & 0xF
		// CV5를 1차원으로 펼쳐 두었으므로 CV5[cv5_group_idx][cv5_sub_idx] == CV5_flat[mtxm & 0x7FFF]
		// 최상위 비트(0x8000)는 OpenBW tile_id::group_index()와 같이 버린다
		cv5_idx = g_mtxm[megatile_pos.y * map_size_megatile.x + megatile_pos.x] & kMTXMTileIdMask;

		// 범위 밖 그룹은 OpenBW와 같이 tile id(MTXM 값) 0으로 대체 (bwgame.h: tile_id = {})
		if (cv5_count <= cv5_idx)
		{
			cv5_idx = 0;
		}
	}
	else
	{
		// Creep 타일: 바닥 전체를 Creep 메가타일로 (OpenBW: cv5[1].mega_tile_index[랜덤])
		// 크립 바닥: CV5 그룹 1의 0~12번 중 로드 시 정해 둔 무작위 번호
		cv5_idx = kCreepCV5Group * kCV5MegatilesPerGroup + g_creep_random_tiles.Load(int3(megatile_pos, 0));
	}

	return g_cv5[cv5_idx];
}

// 맵 밖 타일은 Creep 없음으로 취급 (OpenBW: 범위 밖 이웃은 건너뜀)
bool HasCreep(int2 megatile_pos, int2 map_size_megatile)
{
	bool has_creep = false;
	if (all(megatile_pos >= 0) && all(megatile_pos < map_size_megatile))
	{
		const uint megatile_state = g_megatile_states[megatile_pos.y * map_size_megatile.x + megatile_pos.x];
		has_creep = ((megatile_state >> kMegatileStateFlagsShift) & kMegatileFlagHasCreep) != 0;
	}
	return has_creep;
}

// 메가타일(VX4 인덱스) 안 로컬 픽셀(0~31)의 WPE 인덱스 — SCMapBaker_CS와 같은 조회 체인
uint DecodeMegatilePixel(uint megatile_index, uint2 local)
{
	const uint2 minitile = local / kMinitilePixelSize; // 0~3
	const uint2 px = local % kMinitilePixelSize; // 0~7

	// VX4: bit0 = 좌우 반전, 나머지 = VR4 인덱스
	const uint vx4_entry = g_vx4[megatile_index * (kMinitilesPerMegatileAxis * kMinitilesPerMegatileAxis)
		+ minitile.y * kMinitilesPerMegatileAxis + minitile.x];
	const uint flip = vx4_entry & 0x1;
	const uint vr4_index = vx4_entry >> 1;

	const uint px_x = flip ? (kMinitilePixelSize - 1 - px.x) : px.x;
	return g_vr4[vr4_index * (kMinitilePixelSize * kMinitilePixelSize) + px.y * kMinitilePixelSize + px_x];
}

uint main(SCMapVSOut input) : SV_TARGET
{
	// CV5 Group: 16개의 MegatileIdx로 구성되어 있음
	uint cv5_count = 0;
	g_cv5.GetDimensions(cv5_count);
	
	const uint2 pixel = uint2(floor(input.pixel_coord)); // 맵 UV 범위 안이라 음수 없음
	const int2 megatile_pos = int2(pixel / kMegatilePixelSize); // uint 나눗셈 (32 → 시프트)
	const uint2 local = pixel % kMegatilePixelSize;
	
	const bool has_creep = HasCreep(megatile_pos, input.map_size_megatile);
	
	const uint megatile_idx = ResolveMegatile(megatile_pos, input.map_size_megatile, cv5_count, has_creep);

	// 현재 픽셀의 wpe index
	uint wpe_index = DecodeMegatilePixel(megatile_idx, local);

	// Creep이 아닌 타일: 주변에 Creep이 있는지 확인 후
	// Creep이 있을 시 메가타일 픽셀 위에 테두리 프레임을 커버리지가 있는 픽셀만 덮어씀
	if(has_creep == false)
	{
		// 우하단부터 역래스터 순서(OpenBW 비트 0~7 순서)
		uint neighbor_mask = 0;
		[unroll]	// 반복문을 컴파일 시점에 코드로 싹 펼침
		for (uint i = 0; i < 8; ++i)
		{
			if (HasCreep(megatile_pos + kCreepNeighborOffsets[i], input.map_size_megatile))
			{
				neighbor_mask |= (1u << i);
			}
		}

		const uint edge_frame = kCreepEdgeFrameIndex[neighbor_mask]; // 0 = 테두리 없음
		if (edge_frame != 0)
		{
		// 테두리 GRP 캔버스 = 메가타일 1칸 → 로컬 좌표를 그대로 캔버스 좌표로 사용
			const uint2 edge = g_creep_edge.Load(int4(local, edge_frame - 1, 0));
			if (edge.y != 0)	// G: 커버리지. 크립이 없을 경우 기존 맵의 픽셀(WPE)을 사용
			{
				wpe_index = edge.x;
			}
		}
	}

	return wpe_index;
}