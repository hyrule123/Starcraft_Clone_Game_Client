#ifndef HLSL_SC_SPRITE_DEBUG
#define HLSL_SC_SPRITE_DEBUG

#include <Engine/HLSL/Core/CoreMinimal.hlsli>

#include <Engine/HLSL/Camera.hlsli>
#include <Engine/HLSL/CommonSampler.hlsli>
#include <Engine/HLSL/RenderPass.hlsli>

#define SLOT_T_GRP_SPRITE REG_T_PER_MATERIAL_0

struct SpriteInstanceData
{
	matrix world_mat;
	uint sprite_frame_idx;
	BOOL flip_left_right;
	float2 padding_0;
};

struct SpriteVSInput
{
	float3 position SEMANTIC(POSITION);
	// 위치는 인스턴스 위치와 캔버스 크기, UV로 전부 계산됨 -> position 필요 없음
	float2 uv SEMANTIC(TEXCOORD);
	
#ifdef __HLSL
	uint instance_ID : SV_InstanceID;
#endif //__HLSL
};

#ifdef __HLSL

Texture2DArray g_grp_sprite : register(SLOT_T_GRP_SPRITE);
StructuredBuffer<SpriteInstanceData> g_sprite_instance_data : register(REG_T_INSTANCE_BUFFER);

struct SpritePSInput
{
	float4 position : SV_Position;
	float2 local : TEXCOORD0; // 캔버스 내 로컬 좌표 (보간)
	nointerpolation uint frame : FRAME; // 슬라이스 번호
	nointerpolation uint color : PLAYER_COLOR; // tunit 행 번호 (0~15)
};
#endif//__HLSL

#endif//HLSL_SC_SPRITE_DEBUG