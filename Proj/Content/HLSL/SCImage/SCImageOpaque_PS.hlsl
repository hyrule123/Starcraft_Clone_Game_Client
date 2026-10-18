#include <Content/HLSL/SCImage/SCImageCommon.hlsli>



uint main(ImageVSOutput input) : SV_TARGET
{
	const int2 texel = int2(floor(input.local));
	const uint2 sampled = g_grp_sprite.Load(int4(texel, input.frame, 0));

	if (sampled.y == 0)
	{
		discard; // 커버리지 0 = 투명
	}

	uint index = sampled.x;
	if (index >= 8 && index < 16)
	{
		index = ApplyPlayerColor(index, input.player_color_idx);
	}

	return index;
}