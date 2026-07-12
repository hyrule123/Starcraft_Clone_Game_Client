#include <Content/Shader/SCSprite.hlsli>

VS_OUT main(SpriteVSInput indata)
{
	VS_OUT outdata;
	
	outdata.position = mul(float4(indata.position, 1.0f), g_sprite_instance_data[indata.instance_ID].world_mat);
	outdata.position = mul(outdata.position, g_CB_camera.view_mat);
	outdata.position = mul(outdata.position, g_CB_camera.proj_mat);
	outdata.UV = indata.UV;
	outdata.instance_ID = indata.instance_ID;
	
	return outdata;
}