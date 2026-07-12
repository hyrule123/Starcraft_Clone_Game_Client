#ifndef HLSL_SPRITE
#define HLSL_SPRITE

#include <Engine/HLSL/CppShared/CoreMinimal.hlsli>

struct alignas(16)     SCSpriteInstanceData
{
	matrix world_mat;
	uint sprite_frame_idx;
	float3 padding_0;
};

struct SpriteVSInput
{
	float3 position SEMANTIC(POSITION)
	float2 UV SEMANTIC(TEXCOORD)
	
	#ifdef __HLSL
	uint instance_ID :SV_InstanceID;
	#endif //__HLSL
};

#ifdef __HLSL
#include <Engine/HLSL/CommonConstBuffer.hlsli>
#include <Engine/HLSL/CommonSampler.hlsli>

Texture2DArray base_color : register(SLOT_T_BASE_COLOR);
StructuredBuffer<SCSpriteInstanceData> g_sprite_instance_data : register(SLOT_T_PER_INSTANCE);

struct VS_OUT
{
		float4 position : SV_Position;
		float2 UV : TEXCOORD;
		uint instance_ID : SV_InstanceID;
};
#endif//__HLSL

#endif//HLSL_SPRITE