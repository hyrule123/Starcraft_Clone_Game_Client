#include <Content/HLSL/SCImage/SCHpBar.hlsli>

HpBarVSOutput main(ImageVSInput input)
{
	HpBarVSOutput output;

	const SCHpBarInstance inst = g_sc_hp_bar_insts[input.instance_id];
	
	// UV 값을 반올림하여 정수 corner 값으로 변환. 좌상단 (0, 0), 우하단 (1, 1)
	int2 corner = int2(round(input.uv));
	
	const int2 bar_size = int2(int(inst.bar_width), int(GetHpBarHeight(inst.bar_flags)));
	const int2 bar_corner = inst.sc_lt_pos + corner * bar_size;
	
	// 월드 좌표로 변경(sc_x right, sc_y down -> y 반전(y up)
	output.local = float2(corner * bar_size); // PS에서 floor → 열·행
	const float2 vtx_world = ScToWorld(bar_corner.x, bar_corner.y);
	output.position = mul(float4(vtx_world, 0.0, 1.0), g_camera.view_proj);
	output.position.z = DepthKeyToClipZ(inst.depth_key, output.position.w);
	
	output.bar_flags = inst.bar_flags;
	output.hp_fill = inst.hp_fill;
	output.shield_fill = inst.shield_fill;
	output.energy_fill = inst.energy_fill;
	output.hp_stage = inst.hp_stage;
	
	return output;
}