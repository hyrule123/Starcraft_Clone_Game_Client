#include <Content/HLSL/SCSpriteDebug.hlsli>

SpritePSInput main(SpriteVSInput input)
{
	SpritePSInput output;
	
	output.position = mul(float4(input.position, 1.0f), g_sprite_instance_data[input.instance_ID].world_mat);
	output.position = mul(output.position, g_camera.view_mat);
	output.position = mul(output.position, g_camera.proj_mat);
	
	output.frame = 0;
	output.color = float4(1.0f, 1.0f, 1.0f, 1.0f);
	output.local = input.uv;
	
	// TODO: 계산 로직 추가
	return output;
}