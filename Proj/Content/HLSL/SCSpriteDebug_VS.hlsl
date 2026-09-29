#include <Content/HLSL/SCSpriteDebug.hlsli>

StructuredBuffer<SCSpriteInstance> g_sprite_instances : register(REG_T_INSTANCE_BUFFER);

SpritePSInput main(SpriteVSInput input)
{
	SpritePSInput output;

	const SCSpriteInstance inst = g_sprite_instances[input.instance_ID];

	uint width, height, slices;
	g_grp_sprite.GetDimensions(width, height, slices);

    // 반전은 로컬 좌표를 뒤집어서 처리 (정점 감김 순서 유지)
	const bool flipped = GetSpriteBits(inst.flags, kSpriteFlippedShift, kSpriteFlippedBits) != 0;
	const float u = flipped ? (1.0 - input.uv.x) : input.uv.x;
	output.local = float2(u, input.uv.y) * float2(width, height);

    // SC 좌표에서 캔버스 배치 (OpenBW와 동일한 정수 내림)
	const int2 center = int2(round(inst.position));
	const int2 top_left = center - int2(width >> 1, height >> 1);
	const float2 sc_pos = float2(top_left) + input.uv * float2(width, height);

    // SC → 월드 (엔진 변환 규칙 x 그대로, y 반전. C++ 변환 함수와 같은 규칙)
	const float2 world = float2(sc_pos.x, -sc_pos.y);
	output.position = mul(float4(world, 0.0, 1.0), g_camera.view_proj);

	output.frame = inst.frame;
	output.color = GetSpriteBits(inst.flags, kSpriteColorShift, kSpriteColorBits);
	return output;
}