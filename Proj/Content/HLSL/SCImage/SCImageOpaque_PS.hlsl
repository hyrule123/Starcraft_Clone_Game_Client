#include <Content/HLSL/SCImage/SCImageCommon.hlsli>

uint main(ImageVSOutput input) : SV_TARGET
{
	const int2 texel = int2(floor(input.local));
	const uint2 sampled = SampleGrpOrDiscard(input);
	
	if (input.modifier == kModifierWarpTexture)
	{
   		// 건물 잘린 프레임 좌상단 = 워프 텍스처 (0, 0). 색은 텍스처 원본 값 (플레이어 색 치환 없음)
		const int2 warp_texel = texel - int2(g_grp_frame_offsets[input.frame]);
		return g_warp_texture.Load(int4(warp_texel, input.modifier_data1, 0)).x;
	}

	return ApplyPlayerColor(sampled.x, input.player_color_idx);
}