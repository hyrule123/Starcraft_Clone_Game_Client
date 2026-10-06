#include <Content/HLSL/SCImage/SCImageCommon.hlsli>



float4 main(ImagePSInput input) : SV_TARGET
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
		index = g_tunit[input.color * 8 + (index - 8)];
	}

    // WPE 네 번째 바이트는 패딩이라 알파로 쓰지 않음
	return float4(g_wpe[index].rgb, 1.0);
}