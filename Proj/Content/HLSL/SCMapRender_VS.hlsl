#include <Content/HLSL/SCMapRender.hlsli>

SCMapVSOut main(SCMapVSIn input)
{
	SCMapVSOut output;
	output.pos = float4(input.position, 1.0f);
	output.pos.z = 5.0f;
	
	// 왜인지 작동이 안됨...
	if (any(map_info[0].megatile_size == int2(0, 0)))
	{
		//output.pos.xy = mul(output.pos.xy, (128 * 8));
		//output.pixel_coord = mul(output.pixel_coord, (128 * 8));
	}
	//output.pos.xy = output.pos.xy * (128 * 32);
	//output.pixel_coord = input.uv * (128 * 32);
	
	output.pos.xy = output.pos.xy * map_info[input.instance_id].megatile_size * 32;
	output.pixel_coord = input.uv * map_info[input.instance_id].megatile_size * 32;
	
	//output.pos.xy = mul(output.pos.xy, 128 * 8);
	//output.pos.xy = mul(output.pos.xy, map_info[0].megatile_size);
	//output.pos.xy = mul(output.pos.xy, MEGATILE_PIXEL_SIZE);
	
	//output.pixel_coord = output.pos.xy;
	
	output.pos = mul(output.pos, g_CB_camera.view_mat);
	output.pos = mul(output.pos, g_CB_camera.proj_mat);
	return output;
}