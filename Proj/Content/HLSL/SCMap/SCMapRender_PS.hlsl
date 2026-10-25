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

// 맵 밖 타일은 Creep 없음으로 취급 (OpenBW: 범위 밖 이웃은 건너뜀)
bool HasCreep(int2 tile, int2 map_size)
{
	bool has_creep = false;
	if (all(tile >= 0) && all(tile < map_size))
	{
		const uint tile_state = g_tile_states[tile.y * map_size.x + tile.x];
		has_creep = ((tile_state >> kTileStateFlagsShift) & kTileFlagHasCreep) != 0;
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
	const uint2 pixel = uint2(floor(input.pixel_coord)); // 맵 UV 범위 안이라 음수 없음
	const int2 tile = int2(pixel / kMegatilePixelSize); // uint 나눗셈 (32 → 시프트)
	const uint2 local = pixel % kMegatilePixelSize;

	// 맵 타일 크기 = Creep 랜덤 표 크기 (메가타일 단위, MipLevels 1)
	uint map_width = 0;
	uint map_height = 0;
	g_creep_random_tiles.GetDimensions(map_width, map_height);
	const int2 map_size = int2(map_width, map_height);

	// 1) Creep 타일: 바닥 전체를 Creep 메가타일로 (OpenBW: cv5[1].mega_tile_index[랜덤])
	if (HasCreep(tile, map_size))
	{
		const uint creep_variant = g_creep_random_tiles.Load(int3(tile, 0));
		const uint creep_megatile = g_cv5[kCreepCV5Group * kCV5MegatilesPerGroup + creep_variant];
		return DecodeMegatilePixel(creep_megatile, local);
	}

	// 2) Creep이 아닌 타일: 주변에 Creep이 있는지 확인 후
	// Creep이 있을 시 구운 맵 위에 테두리 프레임을 커버리지가 있는 픽셀만 덮어씀
	uint wpe_index = g_map_texture.Load(int3(int2(pixel), 0));

	// 우하단부터 시작해서 시계방향으로 8방향 이웃을 확인 (OpenBW: ui.h draw_tiles)
	uint neighbor_mask = 0;
	[unroll]
	for (uint i = 0; i < 8; ++i)
	{
		if (HasCreep(tile + kCreepNeighborOffsets[i], map_size))
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

	return wpe_index;
}