#include <Content/HLSL/SCMapTileSet.hlsli>

// 메가타일 = 16 미니타일 (4 * 4)
// 미니타일 = 64 픽셀 (8 * 8)
// 미니타일의 크기 기준으로 스레드 설정 (8x8 = 64픽셀)
[numthreads(8, 8, 1)]
void main(uint3 Gid : SV_GroupID, // 미니타일 좌표
            uint3 GTid : SV_GroupThreadID, // 미니타일 내 픽셀 (0~7)
            uint3 DTid : SV_DispatchThreadID)
{
	// 현재 스레드의 픽셀 번호 계산
	const uint2 map_pixel_size = g_map_info.megatile_size * MEGATILE_PIXEL_SIZE;

	if (DTid.x >= map_pixel_size.x || DTid.y >= map_pixel_size.y)
	{
		return;
	}
	
	// 메가타일 좌표 계산 (1 메가타일 == 4 * 4 미니타일)
	const uint2 megatile_coord = Gid.xy / 4;
	
	// MTXM (맵 데이터)에서 현재 메가타일의 인덱스 조회
	const uint megatile_flat_idx =
        MTXM_CV5_indices[megatile_coord.y * g_map_info.megatile_size.x + megatile_coord.x];
	
	// CV5는 원래 16개의 메가타일 그룹으로 이루어져 있음
	// megatile_group = megatile_flat_idx >> 4u
	// idx_in_group = megatile_flat_idx & 0xFu
	// CV5[megatile_group][megatile_idx]
	
	// 하지만, 현재 코드에서는 CV5를 평탄화하여 1차원 배열로 사용하고 있기 때문에
	// CV5_flat[megatile_flat_idx] 를 통해 즉시 접근이 가능하다.
	const uint vx4_idx = CV5_megatiles[megatile_flat_idx];
	
	// VX4에는 미니타일 16개(4*4) 로 구성된 메가타일 1개의 정보가 들어있다.
	// 자신이 메가타일 속 미니타일 좌표를 구한다.(0 - 3, 0 - 3)
	const uint2 minitile_coord = Gid.xy % 4;
	
	// 2d 배열 -> 1d 배열로 변환하여 조회
	const uint vx4_entry = VX4_minitiles[vx4_idx * 16 + minitile_coord.y * 4 + minitile_coord.x];
	
	// vx4_entry의 첫 비트는 플립 여부(좌우 플립만 존재)
	const uint flip = vx4_entry & 0x1;
	
	// 나머지 비트는 VR4 인덱스
	const uint vr4_idx = vx4_entry >> 1;
	
	// 플립 여부에 따라 픽셀 좌표 계산
	const uint px_x = flip ? (7 - GTid.x) : GTid.x;

	// 최종 결과 = WPE Palette Index(0 ~ 255) - uint8 형태로 저장
	map_output[DTid.xy] =
        VR4_minitile_wpe_indices[vr4_idx * 64 + GTid.y * 8 + px_x];
}