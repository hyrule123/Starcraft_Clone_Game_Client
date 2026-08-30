#include <Content/HLSL/SCMapRender.hlsli>

float4 main(SCMapVSOut input) : SV_TARGET
{
	//반올림
	int2 texel = floor(input.pixel_coord);
	
	uint wpe_index = map_texture.Load(int3(texel, 0));
	
	return wpe_indices[wpe_index];
}