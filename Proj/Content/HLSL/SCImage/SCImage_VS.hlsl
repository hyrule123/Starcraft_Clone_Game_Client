#include <Content/HLSL/SCImage/SCImageCommon.hlsli>

StructuredBuffer<SCImageInstance> g_image_instances : register(REG_T_INSTANCE_BUFFER);

ImageVSOutput main(ImageVSInput input)
{
	ImageVSOutput output;

	const SCImageInstance inst = g_image_instances[input.instance_id];

	// 캔버스
	uint canvas_w, canvas_h, canvas_slices;
	g_grp_sprite.GetDimensions(canvas_w, canvas_h, canvas_slices);
	
	// UV 값을 반올림하여 정수 corner 값으로 변환. 좌상단 (0, 0), 우하단 (1, 1)
	int2 corner = int2(round(input.uv));
	
	const int canvas_w_half = int(canvas_w >> 1);
	const int canvas_h_half = int(canvas_h >> 1);
	
	const bool flipped = inst.image_flags & kImageFlagHorizontallyFlipped;
	
	// 위치만 미러링하면 정점 순서가 뒤집혀 winding이 반대가 됨(backface culling 위험).
	// corner를 먼저 반전하고 위치에서 한 번 더 미러링 → 정점 순서는 원래대로 유지, 샘플링 방향만 반전됨
	if(flipped)
	{
		corner.x = 1 - corner.x;
	}
	
	const GrpFrameRect frame_rect = g_grp_frame_rects[inst.frame];
	
	const int2 frame_corner = int2(frame_rect.offset) + corner * int2(frame_rect.size);
	
	// 텍스처의 로컬 좌표: 반전된 그대로 사용
	output.local = float2(frame_corner);
	
	// 캔버스 중심 기준 상대 위치(로컬)
	const int2 rel = frame_corner - int2(canvas_w_half, canvas_h_half);
	
    // SC 좌표(x right, y down)에서 프레임 배치
	// x축 반전일 경우 중심 기준 미러링(맵 기준)
	const int2 vtx_sc = inst.sc_center_pos + int2(flipped ? -rel.x : rel.x, rel.y);
	
	// 월드 좌표로 변경(x right, y up -> y 반전)
	const float2 vtx_world = ScToWorld(vtx_sc.x, vtx_sc.y);
	
	output.position = mul(float4(vtx_world, 0.0, 1.0), g_camera.view_proj);
	
	output.position.z = float(inst.depth_key + 1u) * kScDepthScale * output.position.w; // SC 자체 정렬 기준이 들어있는 depth_key로 변경한다.

	output.modifier = inst.modifier;
	output.modifier_data1 = inst.modifier_data1;
	output.remapping = inst.remapping;
	output.frame = inst.frame;
	output.player_color_idx = inst.player_color_idx;
	return output;
}