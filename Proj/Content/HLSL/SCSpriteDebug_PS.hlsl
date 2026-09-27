
#include <Content/HLSL/SCSpriteDebug.hlsli>

float4 main(SpritePSInput input) : SV_TARGET
{
	return float4(1.0f, 0.0f, 0.0f, 1.0f);
	
	float4 color = g_grp_sprite.Sample(g_point_clamp_sampler, float3(input.local, input.frame));
	
	return color;
}