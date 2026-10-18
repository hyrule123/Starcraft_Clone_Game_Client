#include <Content/HLSL/SCImage/SCImageCommon.hlsli>

StructuredBuffer<SCImageInstance> g_image_instances : register(REG_T_INSTANCE_BUFFER);

ImagePSInput main(ImageVSInput input)
{
	ImagePSInput output;

	const SCImageInstance inst = g_image_instances[input.instance_id];

	uint width, height, slices;
	g_grp_sprite.GetDimensions(width, height, slices);
	
	// UV 값을 반올림하여 정수 corner 값으로 변환(0 ~ 1)
	const uint2 corner = uint2(round(input.uv));
	
	const int half_w = int(width >> 1);
	const int half_h = int(height >> 1);
	
	// 반전은 로컬 좌표를 뒤집어서 처리 (정점 감김 순서 유지)
	const bool flipped = inst.image_flags & kImageFlagHorizontallyFlipped;
	const float u = flipped ? float(1 - corner.x) : float(corner.x);
	output.local = float2(u, corner.y) * float2(width, height);
	
	// 원작 반전 배치: left = cx + w/2 - w (홀수 너비면 비반전보다 1픽셀 왼쪽)
	const int left = flipped ? (inst.sc_center_pos.x + half_w - int(width)) : (inst.sc_center_pos.x - half_w);
	const int top = inst.sc_center_pos.y - half_h;
	
    // SC 좌표(x right, y down)에서 캔버스 배치
	const int2 vtx_sc = int2(left, top) + int2(corner) * int2(width, height);
	
	// 월드 좌표로 변경(x right, y up -> y 반전)
	const float2 vtx_world = ScToWorld(vtx_sc.x, vtx_sc.y);
	
	output.position = mul(float4(vtx_world, 0.0, 1.0), g_camera.view_proj);
	
	output.position.z = float(inst.depth_key + 1u) * kScDepthScale * output.position.w; // SC 자체 정렬 기준이 들어있는 depth_key로 변경한다.

	output.frame = inst.frame;
	output.color_idx = inst.player_color_idx;
	return output;
}