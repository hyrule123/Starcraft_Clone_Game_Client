#ifndef HLSL_SCMAP
#define HLSL_SCMAP

#include <Content/HLSL/SCMapCommon.hlsli>
#include <Engine/HLSL/Camera.hlsli>
#include <Engine/HLSL/ForwardOpaquePass.hlsli>

#define REG_T_MAP_TEXTURE REG_T_PER_MATERIAL_0
#define REG_T_WPE_INDICES REG_T_PER_PIPELINE_0

struct SCMapVSIn
{
	float3 position SEMANTIC(POSITION);
	float2 uv SEMANTIC(TEXCOORD);
	
	#ifdef __HLSL
	uint instance_id SEMANTIC(SV_InstanceID);
	#endif//__HLSL	
};

#ifdef __HLSL

struct SCMapVSOut
{
	float4 pos SEMANTIC(SV_Position);
	float2 pixel_coord SEMANTIC(TEXCOORD); // 픽셀 단위, UV 아님
};

// SBuffer
Texture2D<uint> map_texture : register(REG_T_MAP_TEXTURE);
Buffer<float4> wpe_indices : register(REG_T_WPE_INDICES);

// CBuffer
// Slot 0: Map Info
cbuffer MapInfo : register(REG_B_MAP_INFO)
{
	MapInfoCB g_map_info;
};

// Slot 1: Camera Info




#endif

#ifdef __cplusplus

#endif


#endif//HLSL_SCMAP