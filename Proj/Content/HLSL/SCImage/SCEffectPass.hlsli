#ifndef SC_EFFECT_PASS_HLSLI
#define SC_EFFECT_PASS_HLSLI
#include <Content/HLSL/SCImage/SCImageCommon.hlsli>

// Modifiers
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

#ifdef __HLSL
RasterizerOrderedTexture2D<uint> g_sc_wpe_rt : register(REG_U_SC_WPE_TO_ROV_SLOT);
RasterizerOrderedTexture2D<uint> g_sc_mirror_rt : register(REG_U_SC_WPE_MIRROR_SLOT);
#endif//__HLSL

#endif//SC_EFFECT_PASS_HLSLI