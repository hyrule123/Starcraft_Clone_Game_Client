#ifndef HLSL_SC_IMAGE_COMMON
#define HLSL_SC_IMAGE_COMMON

#include <Engine/HLSL/Core/CoreMinimal.hlsli>
#include <Engine/HLSL/Camera.hlsli>
#include <Engine/HLSL/CommonSampler.hlsli>
#include <Engine/HLSL/RenderPass.hlsli>
#include <Content/HLSL/SCCommon.hlsli>

#define REG_T_SC_GRP_SPRITE REG_T_PER_MATERIAL_0
#define REG_T_SC_GRP_FRAME_RECTS REG_T_PER_MATERIAL_1

// 건물 완료 시 생성되는 효과에 사용되는 Warp Texture. (Per Frame에 바인딩) -> SCImageRenderer::kModifierWarpTexture
#define REG_T_SC_WARP_TEXTURE REG_T_PER_FRAME_6
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
	// 비트 배치는 MainGameScene.cpp의 depth_key 주석 참고
	uint depth_key;
	
	int frozen_frame_top_y; // flags: YFrozen가 켜졌을 때만 유효. SC 좌표 기준 프레임 상단
	
	uint image_flags;	// SCSim::ImageFlags 원본
	int modifier;		// image_t::modifier
	int modifier_data1; // image_t::modifier_data1
	uint player_color_idx; // players[owner].color (0~15)
	uint remapping;		// ImageDef::remapping (light LUT 번호, modifier 9용)
};
#ifdef __cplusplus
static_assert(sizeof(SCImageInstance) == 40, "SCImageInstance Layout changed.");
#endif

struct GrpFrameRect
{
	uint2 offset;
	uint2 size;
};
#ifdef __cplusplus
static_assert(sizeof(GrpFrameRect) == 16, "GrpFrameRect Layout changed.");
#endif

struct ImageVSInput
{
	// 위치는 인스턴스 위치와 캔버스 크기, UV로 전부 계산됨
	float2 uv SEMANTIC(TEXCOORD);
	
#ifdef __HLSL
	uint instance_id : INSTANCE_ID;
#endif //__HLSL
};

HLSL_CONSTEXPR uint kImageFlagHorizontallyFlipped = 0x2;	// C++ (SCImageRenderer) 쪽에서 검증
HLSL_CONSTEXPR uint kImageFlagYFrozen = 0x4;

// *** Modifiers ***: image_t::modifier 값
//	Effect Pass
HLSL_CONSTEXPR int kModifierCloaking = 2;
HLSL_CONSTEXPR int kModifierCloaked = 3;
HLSL_CONSTEXPR int kModifierDecloaking = 4;
HLSL_CONSTEXPR int kModifierEnemyCloaking = 5;
HLSL_CONSTEXPR int kModifierEnemyCloaked = 6; // 탐지 못 한 적의 은폐 (OpenBW 렌더러 미지원 → 왜곡으로 구현)
HLSL_CONSTEXPR int kModifierEnemyDecloaking = 7;
HLSL_CONSTEXPR int kModifierDistortion = 8;
HLSL_CONSTEXPR int kModifierRemapping = 9;
HLSL_CONSTEXPR int kModifierSelectionCircle = 13;
HLSL_CONSTEXPR int kModifierShadow = 10;

HLSL_CONSTEXPR int kModifierWarpFlash = 17;

//	Opaque Pass
HLSL_CONSTEXPR int kModifierWarpTexture = 12;
// ****************

#ifdef __HLSL
struct ImageVSOutput
{
	float4 position : SV_Position;
	float2 local : TEXCOORD0; // 캔버스 내 로컬 좌표 (보간) - Grp에서 texel 추출 시 사용됨(UV에 대응)
	nointerpolation uint frame : FRAME; // 슬라이스 번호
	nointerpolation int modifier : MODIFIER; // image_t::modifier
	nointerpolation int modifier_data1 : MODIFIER_DATA1; // image_t::modifier_data1
	nointerpolation uint remapping : REMAPPING;
	nointerpolation uint player_color_idx : PLAYER_COLOR; // tunit 행 번호 (0~15)
};

Texture2DArray<uint2> g_grp_sprite : register(REG_T_SC_GRP_SPRITE);
Texture2DArray<uint2> g_warp_texture : register(REG_T_SC_WARP_TEXTURE);

Buffer<uint> g_common_palettes : register(REG_T_SC_COMMON_PALETTES); // R8_UINT, tunit, tselect 등. 상수 오프셋 값을 통해 조회
Buffer<float4> g_wpe : register(REG_T_SC_WPE_PALETTE); // R8G8B8A8_UNORM, 256개

// GRP의 텍스처(array)별 캔버스 내 프레임의 오프셋, 크기
StructuredBuffer<GrpFrameRect> g_grp_frame_rects : register(REG_T_SC_GRP_FRAME_RECTS);

float DepthKeyToClipZ(uint depth_key, float clip_w)
{
	return float(depth_key + 1u) * kScDepthScale * clip_w;
}

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
		index = g_common_palettes[kPaletteTUnitOffset + color_idx * 8 + (index - 8)];
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