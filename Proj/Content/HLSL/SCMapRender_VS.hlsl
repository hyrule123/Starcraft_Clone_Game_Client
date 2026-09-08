#include <Content/HLSL/SCMapRender.hlsli>

SCMapVSOut main(SCMapVSIn input)
{
	SCMapVSOut output;
	output.pos = float4(input.position, 1.0f);
	output.pos.z = 2.0f;
	
	output.pos.xy = output.pos.xy * g_map_info.megatile_size * 32;
	output.pixel_coord = input.uv * g_map_info.megatile_size * 32;
	
	//output.pos.xy = mul(output.pos.xy, 128 * 8);
	//output.pos.xy = mul(output.pos.xy, map_info[0].megatile_size);
	//output.pos.xy = mul(output.pos.xy, MEGATILE_PIXEL_SIZE);
	
	//output.pixel_coord = output.pos.xy;
	
	output.pos = mul(output.pos, g_camera.view_mat);
	output.pos = mul(output.pos, g_camera.proj_mat);
	return output;
}