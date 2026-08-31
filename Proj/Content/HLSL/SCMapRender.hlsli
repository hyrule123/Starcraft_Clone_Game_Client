#ifndef HLSL_SCMAP
#define HLSL_SCMAP

#include <Content/HLSL/SCMapCommon.hlsli>

#define SLOT_T_MAP_TEXTURE REG_T(0)
#define SLOT_T_WPE_INDICES REG_T(1)

struct SCMapVSIn
{
	float3 position SEMANTIC(POSITION)
	float2 uv SEMANTIC(TEXCOORD)
	
	#ifdef __HLSL
	uint instance_id SEMANTIC(SV_InstanceID)
	#endif//__HLSL
	
};

#ifdef __HLSL
#include <Engine/HLSL/CommonConstBuffer.hlsli>

struct SCMapVSOut
{
	float4 pos SEMANTIC(SV_Position)
	float2 pixel_coord SEMANTIC(TEXCOORD) // 픽셀 단위, UV 아님
};

// SBuffer
Texture2D<uint> map_texture : register(SLOT_T_MAP_TEXTURE); // Slot 0
Buffer<float4> wpe_indices : register(SLOT_T_WPE_INDICES); // Slot 1

// Slot 32: Map Info
StructuredBuffer<MapInfoCB> map_info : register(SLOT_T_PER_INSTANCE);

// CBuffer
// Slot 0: Map Info
// Slot 1: Camera Info



#endif

#ifdef __cplusplus

#endif


#endif//HLSL_SCMAP