#ifndef SC_COMMON_HLSLI
#define SC_COMMON_HLSLI

#include <Engine/HLSL/Core/CoreMinimal.hlsli>

#define REG_T_SC_TUNIT			REG_T_PER_FRAME_5
#define REG_T_SC_WPE_PALETTE	REG_T_PER_FRAME_6

#define REG_B_SC_COMMON_INFO	REG_B_PER_FRAME_1

// CV5 그룹 하나의 메가타일 수 (MTXM 값 = 그룹 << 4 | 그룹 내 번호)
HLSL_CONSTEXPR uint kCV5MegatilesPerGroup = 16;
HLSL_CONSTEXPR uint kMegatilePixelSize = 32;

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

#endif//SC_COMMON_HLSLI