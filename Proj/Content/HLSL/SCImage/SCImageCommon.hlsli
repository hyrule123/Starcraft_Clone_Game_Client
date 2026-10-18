#ifndef HLSL_SC_IMAGE_COMMON
#define HLSL_SC_IMAGE_COMMON

#include <Engine/HLSL/Core/CoreMinimal.hlsli>
#include <Engine/HLSL/Camera.hlsli>
#include <Engine/HLSL/CommonSampler.hlsli>
#include <Engine/HLSL/RenderPass.hlsli>

#define REG_T_SC_GRP_SPRITE REG_T_PER_MATERIAL_0
#define REG_T_SC_TUNIT			REG_T_PER_FRAME_5
#define REG_T_SC_WPE_PALETTE	REG_T_PER_FRAME_6
#define REG_T_SC_PCX_TILESET_LUT	REG_T_PER_FRAME_7
#define REG_U_SC_WPE_TO_ROV_SLOT REG_U(0)

// C++ / HLSL 공유
struct SCImageInstance
{
	int2 sc_center_pos; // SC 좌표계 기준 캔버스 중앙 위치
	uint frame; // Texture2DArray 슬라이스
	uint flags; // 기존 비트 배치 그대로(비트 배치는 아래 주석 참고)
	
	// 깊이 키. VS에서 z = (depth_key + 1) * 2^-24 로 변환해 SV_Position.z에 덮어씀 (+1: 맵이 z = 0 사용)
	// 범위 0 ~ 16777215 (24비트) — 이 범위에서 float32로 오차 없이 표현됨
	// 현재 16비트 사용: [rank 12 (상위)][order 4 (하위)], 생성은 MainGameScene의 MakeDepthKey 참고
	uint depth_key; 
	
	// flags 비트 배치
	// [0]      반전
	// [1..4]   플레이어 색 번호 (0~15)
	// [8..15]  modifier (효과 패스용, 지금은 0)
	// [16..23] modifier_data1 (효과 패스용)
	// [24..27] LUT 테이블 번호 (효과 패스용)
};

#ifdef __HLSL
#define SC_CONST static const
#else
#define SC_CONST constexpr
#endif
SC_CONST uint kSpriteFlippedShift = 0;
SC_CONST uint kSpriteFlippedBits = 1;
SC_CONST uint kSpriteColorShift = 1;
SC_CONST uint kSpriteColorBits = 4;
SC_CONST uint kSpriteModifierShift = 8;
SC_CONST uint kSpriteModifierBits = 8;
SC_CONST uint kSpriteModifierDataShift = 16;
SC_CONST uint kSpriteModifierDataBits = 8;
SC_CONST uint kSpriteLutShift = 24;
SC_CONST uint kSpriteLutBits = 4;

SC_CONST uint kShadowDarkRow = 18; // OpenBW 하드 코딩된 항목
SC_CONST uint kPcxLutTableWidth = 256;
SC_CONST uint kPcxLutDarkOffset = 0;


#ifdef __cplusplus
// 구간이 겹치지 않는지 컴파일 시 확인
static_assert(sizeof(SCImageInstance) == 20, "SCImageInstance is not 20 bytes");
static_assert(kSpriteFlippedShift + kSpriteFlippedBits <= kSpriteColorShift);
static_assert(kSpriteColorShift + kSpriteColorBits <= kSpriteModifierShift);
static_assert(kSpriteModifierShift + kSpriteModifierBits <= kSpriteModifierDataShift);
static_assert(kSpriteModifierDataShift + kSpriteModifierDataBits <= kSpriteLutShift);
static_assert(kSpriteLutShift + kSpriteLutBits <= 32);

#endif

// 깊이 키 → z 변환 배율 (2^-24). z = (key + 1) * kScDepthScale, 키 24비트 이내에서 정확히 표현
SC_CONST float kScDepthScale = 1.0f / 16777216.0f;

struct ImageVSInput
{
	// 위치는 인스턴스 위치와 캔버스 크기, UV로 전부 계산됨
	float2 uv SEMANTIC(TEXCOORD);
	
#ifdef __HLSL
	uint instance_id : INSTANCE_ID;
#endif //__HLSL
};

#ifdef __HLSL
struct ImagePSInput
{
	float4 position : SV_Position;
	float2 local : TEXCOORD0; // 캔버스 내 로컬 좌표 (보간)
	nointerpolation uint frame : FRAME; // 슬라이스 번호
	nointerpolation uint color : PLAYER_COLOR; // tunit 행 번호 (0~15)
};

Texture2DArray<uint2> g_grp_sprite : register(REG_T_SC_GRP_SPRITE);
Buffer<uint> g_tunit : register(REG_T_SC_TUNIT); // R8_UINT, 128개
Buffer<float4> g_wpe : register(REG_T_SC_WPE_PALETTE); // R8G8B8A8_UNORM, 256개
Buffer<uint> g_pcx_tileset_lut : register(REG_T_SC_PCX_TILESET_LUT); // R8_UINT

uint GetSpriteBits(uint flags, uint shift, uint bits)
{
	return (flags >> shift) & ((1u << bits) - 1u);
}

float2 ScToWorld(float sc_x, float sc_y)
{
	return float2(sc_x, -sc_y);
}
float2 ScToWorld(int sc_x, int sc_y)
{
	return float2(float(sc_x), float(-sc_y));
}

#endif//__HLSL

#endif//HLSL_SC_IMAGE_COMMON