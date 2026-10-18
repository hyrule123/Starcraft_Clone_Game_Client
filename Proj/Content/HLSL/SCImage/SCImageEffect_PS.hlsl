#include <Content/HLSL/SCImage/SCImageCommon.hlsli>

RasterizerOrderedTexture2D<uint> g_sc_index_rt : register(REG_U_SC_WPE_TO_ROV_SLOT);

[earlydepthstencil]
void main(ImageVSOutput input)
{
	const int2 texel = int2(floor(input.local));
	const uint2 sampled = g_grp_sprite.Load(int4(texel, input.frame, 0));

	if (sampled.y == 0)
	{
		discard; // 커버리지 0 = 투명
	}
	
	switch(input.modifier)
	{
		case kModifierShadow:	// 그림자
		{
			const uint2 pixel = uint2(input.position.xy);
			const uint old = g_sc_index_rt[pixel];
			g_sc_index_rt[pixel] = g_pcx_tileset_lut[kPcxLutDarkOffset + kShadowDarkRow * kPcxLutTableWidth + old];
			break;
		}
		default:
		{
			uint index = sampled.x;
			g_sc_index_rt[uint2(input.position.xy)] = ApplyPlayerColor(index, input.color_idx);
			break;
		}
	}
}