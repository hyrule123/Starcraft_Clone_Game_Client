#include <Content/HLSL/SCMap/SCMapRender.hlsli>

SCMapVSOut main(SCMapVSIn input)
{
	SCMapVSOut output;
	
	const MapInfoCB map_info = g_map_info[input.instance_id];
	
	output.pos = mul(float4(input.position, 1.0f), map_info.world_mat);
	output.pos = mul(output.pos, g_camera.view_proj);
	output.pos.z = 0.0f;
	
	//output.pos.xy = output.pos.xy * g_map_info.megatile_size * 32;
	output.pixel_coord = input.uv * map_info.map_size_megatile * kMegatilePixelSize;
	output.map_size_megatile = map_info.map_size_megatile;

	return output;
}