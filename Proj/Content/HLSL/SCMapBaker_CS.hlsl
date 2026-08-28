#include <Content/HLSL/SCTileSet.hlsli>

// 미니타일의 크기 기준으로 스레드 설정 (8x8 = 64픽셀)
[numthreads(8, 8, 1)]
void main(uint3 Gid : SV_GroupID, // 미니타일 좌표
            uint3 GTid : SV_GroupThreadID, // 미니타일 내 픽셀 (0~7)
            uint3 DTid : SV_DispatchThreadID)
{
	const uint2 map_pixel_size = g_CB_map_info.megatile_size * 32;

	if (DTid.x >= map_pixel_size.x || DTid.y >= map_pixel_size.y)
	{
		return;
	}

    // 미니타일 좌표 → 타일 좌표 + 메가타일 내 위치
	const uint2 megatile = Gid.xy / 4;
	const uint2 mini = Gid.xy % 4;

	const uint megatile_flat_idx =
        MTXM_CV5_indices[megatile.y * g_CB_map_info.megatile_size.x + megatile.x];

	const uint vx4_idx = CV5_megatiles[megatile_flat_idx + mini.y * 4 + mini.x];
	const uint vx4_entry = VX4_minitiles[vx4_idx];
	const uint flip = vx4_entry & 0x1;
	const uint vr4_idx = vx4_entry >> 1;

	const uint px_x = flip ? (7 - GTid.x) : GTid.x;

	map_output[DTid.xy] =
        VR4_minitile_wpe_indices[vr4_idx * 64 + GTid.y * 8 + px_x];
}