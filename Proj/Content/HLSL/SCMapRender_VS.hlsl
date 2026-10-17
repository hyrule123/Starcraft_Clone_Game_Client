#include <Content/HLSL/SCMapRender.hlsli>

SCMapVSOut main(SCMapVSIn input)
{
	SCMapVSOut output;
	
	output.pos = mul(float4(input.position, 1.0f), g_map_info[0].world_mat);
	output.pos = mul(output.pos, g_camera.view_proj);
	output.pos.z = 0.0f;
	
	//output.pos.xy = output.pos.xy * g_map_info.megatile_size * 32;
	output.pixel_coord = input.uv * g_map_info[0].megatile_size * 32;

	return output;
}