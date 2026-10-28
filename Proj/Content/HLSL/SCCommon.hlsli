#ifndef SC_COMMON_HLSLI
#define SC_COMMON_HLSLI

#include <Engine/HLSL/Core/CoreMinimal.hlsli>

// 맵, 이미지 공용 레지스터 모음

#define REG_B_SC_COMMON_INFO		REG_B_PER_FRAME_1

#define REG_T_SC_COMMON_PALETTES	REG_T_PER_FRAME_4
#define REG_T_SC_WPE_PALETTE		REG_T_PER_FRAME_5

// C++ / HLSL 공유. CBuffer 전용이므로 크기는 16바이트 배수 유지
struct SCCommonInfoCB
{
	// 게임 뷰 영역 (인덱스 RT 픽셀 좌표 기준)
	// 지금은 RT 전체(0, 0, kResolutionWidth, kResolutionHeight)와 같지만,
	// 추후 UI·레터박스로 RT 일부만 게임 뷰가 될 수 있어 별도로 둔다.
	uint2 view_origin; // 좌상단
	uint2 view_size; // 폭, 높이
};

#ifdef __cplusplus
static_assert(sizeof(SCCommonInfoCB) % 16 == 0, "SCCommonInfoCB must be a multiple of 16 bytes");
#endif

#ifdef __HLSL
cbuffer CB_SCCommonInfo : register(REG_B_SC_COMMON_INFO)
{
	SCCommonInfoCB g_sc_common_info;
}
#endif

// 깊이 키 → z 변환 배율 (2^-24). z = (key + 1) * kScDepthScale, 키 24비트 이내에서 정확히 표현
HLSL_CONSTEXPR float kScDepthScale = 1.0f / 16777216.0f;

// CV5 그룹 하나의 메가타일 수 (MTXM 값 = 그룹 << 4 | 그룹 내 번호)
HLSL_CONSTEXPR uint kCV5MegatilesPerGroup = 16;
HLSL_CONSTEXPR uint kMegatilePixelSize = 32;

// Common Palette 조회 오프셋
HLSL_CONSTEXPR uint kPaletteTUnitOffset = 0; // tunit.pcx   16행 × 8
HLSL_CONSTEXPR uint kPaletteTUnitCount = 128;
HLSL_CONSTEXPR uint kPaletteTSelectOffset = kPaletteTUnitOffset + kPaletteTUnitCount; // 128, tselect.pcx 24
HLSL_CONSTEXPR uint kPaletteTSelectCount = 24;
HLSL_CONSTEXPR uint kPaletteTHpBarOffset = kPaletteTSelectOffset + kPaletteTSelectCount; // 152, thpbar.pcx 19
HLSL_CONSTEXPR uint kPaletteTHpBarCount = 19;
HLSL_CONSTEXPR uint kPaletteTotalCount = kPaletteTHpBarOffset + kPaletteTHpBarCount; // 171

#endif//SC_COMMON_HLSLI