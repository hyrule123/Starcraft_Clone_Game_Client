#include <Content/HLSL/SCImage/SCImageCommon.hlsli>

RasterizerOrderedTexture2D<uint> test : register(u1);

float4 main(ImagePSInput input) : SV_TARGET
{
	const uint value = test[int2(input.position.xy)]; // 바인딩 없으면 0
	return float4(1.0f, 1.0f, 1.0f, 1.0f) + value; // 값을 사용해서 제거되지 않게
	
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