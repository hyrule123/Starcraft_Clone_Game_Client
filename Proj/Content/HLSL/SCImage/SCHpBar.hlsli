#ifndef HLSL_SC_HP_BAR
#define HLSL_SC_HP_BAR
#include <Content/HLSL/SCImage/SCImageCommon.hlsli>

#ifdef __HLSL
struct HpBarVSOutput
{
	float4 position : SV_Position;
	float2 local : TEXCOORD0; // 바 안의 픽셀 좌표 (0 ~ w, 0 ~ h)
	nointerpolation uint bar_flags : BAR_FLAGS;
	nointerpolation uint hp_fill : HP_FILL;
	nointerpolation uint shield_fill : SHIELD_FILL;
	nointerpolation uint energy_fill : ENERGY_FILL;
	nointerpolation uint hp_stage : HP_STAGE;
};
#endif//__HLSL

struct SCHpBarInstance
{
	uint bar_width; // bar_height는 플래그를 통해 정해짐
	uint bar_flags;
	int2 sc_lt_pos;
	
	// fill: 픽셀 폭, 3px 단위로 반올림한 값
	uint hp_fill;
	uint shield_fill;
	uint energy_fill;
	
	// kHpBarStageHigh = 0(≥66, 초록), Mid = 1(≥33, 노랑), Low = 2(빨강)
	uint hp_stage;
	
	uint depth_key;
};
HLSL_CONSTEXPR uint kHpBarFlagShield = 0x1; // unit_type->has_shield
HLSL_CONSTEXPR uint kHpBarFlagEnergy = 0x2; // 에너지 보유 || 환영 || 브루들링 (타이머 바 포함)

HLSL_CONSTEXPR uint kHpBarBaseHeight = 5;
HLSL_CONSTEXPR uint kHpBarShieldExtraHeight = 2;
HLSL_CONSTEXPR uint kHpBarEnergyExtraHeight = 6;

// thpbar.pcx 색 묶음 순서와 동일 (OpenBW draw_health_bars의 colors_66 / colors_33 / colors_0)
HLSL_CONSTEXPR uint kHpBarStageHigh = 0;	// >= 66% 초록
HLSL_CONSTEXPR uint kHpBarStageMid  = 1;	// >= 33% 노랑
HLSL_CONSTEXPR uint kHpBarStageLow  = 2;	// < 33%  빨강

// 함수도 C++와 공유 중
inline uint GetHpBarHeight(uint bar_flags)
{
	uint height = kHpBarBaseHeight;
	if (bar_flags & kHpBarFlagShield)
	{
		height += kHpBarShieldExtraHeight;
	}
	if (bar_flags & kHpBarFlagEnergy)
	{
		height += kHpBarEnergyExtraHeight;
	}
	return height;
}



#ifdef __cplusplus
static_assert(sizeof(SCHpBarInstance) == 36, "SCHpBarInstance Layout changed.");
#endif//__cplusplus

#ifdef __HLSL
StructuredBuffer<SCHpBarInstance> g_sc_hp_bar_insts : register(REG_T_INSTANCE_BUFFER);

// OpenBW draw_health_bars
static const uint kHpColorsNoShield[4][5] = //OpenBW = no_shield_colors_
{
	{ 18, 0, 1, 2, 18 },	// High  (no_shield_colors_66)
	{ 18, 3, 4, 5, 18 },	// Mid   (no_shield_colors_33)
	{ 18, 6, 7, 8, 18 },	// Low   (no_shield_colors_0)
	{ 18, 15, 16, 17, 18 } // no_shield_colors_bg (에너지 행 배경도 이것을 사용)
};

static const uint kHpColorsWithShield[4][7] =
{
	{ 18, 0, 0, 1, 1, 2, 18 }, // High  (with_shield_colors_66)
	{ 18, 3, 3, 4, 4, 5, 18 }, // Mid   (with_shield_colors_33)
	{ 18, 6, 6, 7, 7, 8, 18 }, // Low   (with_shield_colors_0)
	{ 18, 15, 15, 16, 16, 17, 18 } // with_shield_colors_bg
};
static const uint kHpColorsBgRow = 3; // kHpColorsNoShield, kHpColorsWithShield 공통 (High / Mid / Low / Bg)

static const uint kShieldColors[2][4] =
{
	{ 18, 10, 11, 18 }, // Fill (shield_colors)
	{ 18, 16, 17, 18 }, // Bg   (shield_colors_bg)
};
static const uint kShieldColorsFillRow = 0; // kShieldColors[0][y]
static const uint kShieldColorsBgRow = 1; // kShieldColors[1][y]

static const uint kEnergyColors[2][5] = 
{ 
	{ 18, 12, 13, 14, 18 },
	{ 18, 15, 16, 17, 18 } // 배경은 kHpColorsNoShield[bg_row(3)]와 동일
}; 
static const uint kEnergyColorsFillRow = 0; // kEnergyColors[0][y]
static const uint kEnergyColorsBgRow = 1; // kEnergyColors[1][y]

static const uint kHpBarBorderColor = 18; // 3픽셀 구분선

static const uint kHpBarShieldRowCount = 4; // 쉴드 행 수

#endif//__HLSL

#endif//HLSL_SC_HP_BAR