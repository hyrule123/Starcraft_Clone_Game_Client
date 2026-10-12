#ifndef HLSL_SC_SPRITE_DEBUG
#define HLSL_SC_SPRITE_DEBUG

#include <Engine/HLSL/Core/CoreMinimal.hlsli>

#include <Engine/HLSL/Camera.hlsli>
#include <Engine/HLSL/CommonSampler.hlsli>
#include <Engine/HLSL/RenderPass.hlsli>

#define REG_T_SC_GRP_SPRITE_DEBUG REG_T_PER_MATERIAL_0
#define REG_T_SC_TUNIT_DEBUG			REG_T_PER_FRAME_5
#define REG_T_SC_WPE_PALETTE_DEBUG	REG_T_PER_FRAME_6
#define REG_T_SC_PCX_TILESET_LUT_DEBUG	REG_T_PER_FRAME_7

// C++ / HLSL 공유 (16바이트)
struct ScDebugSpriteInstance
{
	float2 position; // 캔버스 중앙 기준 월드 위치
	uint frame; // Texture2DArray 슬라이스
	uint flags; // 비트 배치는 아래 참고
	uint depth_key; // 하위 23비트 사용
	
	// flags 비트 배치
	// [0]      반전
	// [1..4]   플레이어 색 번호 (0~15)
	// [8..15]  modifier (효과 패스용, 지금은 0)
	// [16..23] modifier_data1 (효과 패스용)
	// [24..27] LUT 테이블 번호 (효과 패스용)
};

struct ScDebugSpriteVSInput
{
	// 캔버스 중앙 기준 SC 좌표(y 아래로 증가)
	float3 position SEMANTIC(POSITION);
	// 위치는 인스턴스 위치와 캔버스 크기, UV로 전부 계산됨 -> position 필요 없음
	float2 uv SEMANTIC(TEXCOORD);
	
#ifdef __HLSL
	uint instance_id : INSTANCE_ID;
#endif //__HLSL
};

#ifdef __HLSL
#define SC_CONST static const uint
#else
#define SC_CONST constexpr uint
#endif
SC_CONST kSpriteFlippedShiftDebug = 0;
SC_CONST kSpriteFlippedBitsDebug = 1;
SC_CONST kSpriteColorShiftDebug = 1;
SC_CONST kSpriteColorBitsDebug = 4;
SC_CONST kSpriteModifierShiftDebug = 8;
SC_CONST kSpriteModifierBitsDebug = 8;
SC_CONST kSpriteModifierDataShiftDebug = 16;
SC_CONST kSpriteModifierDataBitsDebug = 8;
SC_CONST kSpriteLutShiftDebug = 24;
SC_CONST kSpriteLutBitsDebug = 4;

#ifdef __cplusplus
// 구간이 겹치지 않는지 컴파일 시 확인
static_assert(sizeof(ScDebugSpriteInstance) == 20, "ScDebugSpriteInstance is not 20 bytes");
static_assert(kSpriteFlippedShiftDebug + kSpriteFlippedBitsDebug <= kSpriteColorShiftDebug);
static_assert(kSpriteColorShiftDebug + kSpriteColorBitsDebug <= kSpriteModifierShiftDebug);
static_assert(kSpriteModifierShiftDebug + kSpriteModifierBitsDebug <= kSpriteModifierDataShiftDebug);
static_assert(kSpriteModifierDataShiftDebug + kSpriteModifierDataBitsDebug <= kSpriteLutShiftDebug);
static_assert(kSpriteLutShiftDebug + kSpriteLutBitsDebug <= 32);
#endif

#ifdef __HLSL

struct ScDebugSpritePSInput
{
	float4 position : SV_Position;
	float2 local : TEXCOORD0; // 캔버스 내 로컬 좌표 (보간)
	nointerpolation uint frame : FRAME; // 슬라이스 번호
	nointerpolation uint color : PLAYER_COLOR; // tunit 행 번호 (0~15)
};

Texture2DArray<uint2> g_sc_sprite_debug : register(REG_T_SC_GRP_SPRITE_DEBUG);

uint GetSpriteBitsDebug(uint flags, uint shift, uint bits)
{
	return (flags >> shift) & ((1u << bits) - 1u);
}

#endif//__HLSL

#endif//HLSL_SC_SPRITE_DEBUG