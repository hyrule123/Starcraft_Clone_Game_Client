#ifndef HLSL_SC_IMAGE_COMMON
#define HLSL_SC_IMAGE_COMMON

#include <Engine/HLSL/Core/CoreMinimal.hlsli>
#include <Engine/HLSL/Camera.hlsli>
#include <Engine/HLSL/CommonSampler.hlsli>
#include <Engine/HLSL/RenderPass.hlsli>
#include <Content/HLSL/SCCommon.hlsli>

#define REG_T_SC_GRP_SPRITE REG_T_PER_MATERIAL_0
#define REG_T_SC_GRP_FRAME_OFFSETS REG_T_PER_MATERIAL_1

// 건물 완료 시 생성되는 효과에 사용되는 Warp Texture. (Per Frame에 바인딩) -> SCImageRenderer::kModifierWarpTexture
#define REG_T_SC_WARP_TEXTURE REG_T_PER_FRAME_4
#define REG_T_SC_PCX_TILESET_LUT	REG_T_PER_FRAME_7
#define REG_T_SC_PCX_TILESET_LUT_OFFSETS REG_T_PER_FRAME_8
#define REG_T_SC_CLOAK_FADE_SELECTOR REG_T_PER_FRAME_9

#define REG_U_SC_WPE_TO_ROV_SLOT REG_U(0)
#define REG_U_SC_WPE_MIRROR_SLOT REG_U(1)

// C++ / HLSL 공유
struct SCImageInstance
{
	int2 sc_center_pos; // SC 좌표계 기준 캔버스 중앙 위치
	uint frame; // Texture2DArray 슬라이스
	
	// 깊이 키. VS에서 z = (depth_key + 1) * 2^-24 로 변환해 SV_Position.z에 덮어씀 (+1: 맵이 z = 0 사용)
	// 범위 0 ~ 16777215 (24비트) — 이 범위에서 float32로 오차 없이 표현됨
	// 현재 16비트 사용: [rank 12 (상위)][order 4 (하위)], 생성은 MainGameScene의 MakeDepthKey 참고
	uint depth_key; 
	
	uint image_flags;	// SCSim::ImageFlags 원본
	int modifier;		// image_t::modifier
	int modifier_data1; // image_t::modifier_data1
	uint player_color_idx; // players[owner].color (0~15)
	uint remapping;		// ImageDef::remapping (light LUT 번호, modifier 9용)
};

#ifdef __cplusplus
// 구간이 겹치지 않는지 컴파일 시 확인
static_assert(sizeof(SCImageInstance) == 36, "SCImageInstance is not 36 bytes");
#endif

#ifdef __HLSL
#define SC_CONST static const
#else
#define SC_CONST constexpr
#endif


// 깊이 키 → z 변환 배율 (2^-24). z = (key + 1) * kScDepthScale, 키 24비트 이내에서 정확히 표현
SC_CONST float kScDepthScale = 1.0f / 16777216.0f;

SC_CONST uint kImageFlagHorizontallyFlipped = 0x2;	// C++ (SCImageRenderer) 쪽에서 검증



// Modifiers
//	Effect Pass
SC_CONST int kModifierCloaking = 2;
SC_CONST int kModifierCloaked = 3;
SC_CONST int kModifierDecloaking = 4;
SC_CONST int kModifierEnemyCloaking = 5;
SC_CONST int kModifierEnemyCloaked = 6; // 탐지 못 한 적의 은폐 (OpenBW 렌더러 미지원 → 왜곡으로 구현)
SC_CONST int kModifierEnemyDecloaking = 7;
SC_CONST int kModifierDistortion = 8;
SC_CONST int kModifierRemapping = 9;
SC_CONST int kModifierShadow = 10; // image_t::modifier 값
SC_CONST int kModifierWarpFlash = 17;

//	Opaque Pass
SC_CONST int kModifierWarpTexture = 12;

struct ImageVSInput
{
	// 위치는 인스턴스 위치와 캔버스 크기, UV로 전부 계산됨
	float2 uv SEMANTIC(TEXCOORD);
	
#ifdef __HLSL
	uint instance_id : INSTANCE_ID;
#endif //__HLSL
};

#ifdef __HLSL
struct ImageVSOutput
{
	float4 position : SV_Position;
	float2 local : TEXCOORD0; // 캔버스 내 로컬 좌표 (보간)
	nointerpolation uint frame : FRAME; // 슬라이스 번호
	nointerpolation int modifier : MODIFIER; // image_t::modifier
	nointerpolation int modifier_data1 : MODIFIER_DATA1; // image_t::modifier_data1
	nointerpolation uint remapping : REMAPPING;
	nointerpolation uint player_color_idx : PLAYER_COLOR; // tunit 행 번호 (0~15)
};

Texture2DArray<uint2> g_grp_sprite : register(REG_T_SC_GRP_SPRITE);
Texture2DArray<uint2> g_warp_texture : register(REG_T_SC_WARP_TEXTURE);

Buffer<uint> g_tunit : register(REG_T_SC_TUNIT); // R8_UINT, 128개
Buffer<float4> g_wpe : register(REG_T_SC_WPE_PALETTE); // R8G8B8A8_UNORM, 256개
// GRP 프레임별 캔버스 내 offset (x, y).
Buffer<uint2> g_grp_frame_offsets : register(REG_T_SC_GRP_FRAME_OFFSETS);

float2 ScToWorld(float sc_x, float sc_y)
{
	return float2(sc_x, -sc_y);
}
float2 ScToWorld(int sc_x, int sc_y)
{
	return float2(float(sc_x), float(-sc_y));
}

uint ApplyPlayerColor(uint index, uint color_idx)
{
	if (index >= 8 && index < 16)
	{
		index = g_tunit[color_idx * 8 + (index - 8)];
	}
	return index;
}

// GRP 샘플링 + 투명 픽셀 discard. 반환값 x = WPE 인덱스(또는 효과 값), y = 커버리지(항상 != 0)
// Distortion Read/Sync는 이 함수의 커버리지 조건을 공유해야 B가 A와 어긋나지 않는다.
uint2 SampleGrpOrDiscard(ImageVSOutput input)
{
	const int2 texel = int2(floor(input.local));
	const uint2 sampled = g_grp_sprite.Load(int4(texel, input.frame, 0));

	if (sampled.y == 0)
	{
		discard;
	}
	return sampled;
}

#endif//__HLSL

#endif//HLSL_SC_IMAGE_COMMON