#include <Content/HLSL/SCImage/SCImageCommon.hlsli>

uint main(ImageVSOutput input) : SV_TARGET
{
	const int2 texel = int2(floor(input.local));
	const uint2 sampled = SampleGrpOrDiscard(input);
	
	if (input.modifier == kModifierWarpTexture)
	{
   		// 건물 잘린 프레임 좌상단 = 워프 텍스처 (0, 0). 색은 텍스처 원본 값 (플레이어 색 치환 없음)
		const int2 warp_texel = texel - int2(g_grp_frame_rects[input.frame].offset);
		
		// modifier_data1에 프레임 번호가 들어 있음
		return g_warp_texture.Load(int4(warp_texel, input.modifier_data1, 0)).x;
	}
	else if (input.modifier == kModifierSelectionCircle)
	{
		// 원작(OpenBW 람다와 동일): 0~7만 치환, 나머지는 원본 그대로
		// player_color_idx = tselect 관계 그룹 번호 (SCSelectionCircleObj에서 설정)
		if (sampled.x < 8)
		{
			return g_common_palettes[kPaletteTSelectOffset + input.player_color_idx * 8 + sampled.x];
		}
		return sampled.x;
	}

	return ApplyPlayerColor(sampled.x, input.player_color_idx);
}