#ifndef SC_EFFECT_PASS_HLSLI
#define SC_EFFECT_PASS_HLSLI
#include <Content/HLSL/SCImage/SCImageCommon.hlsli>

#ifdef __HLSL
RasterizerOrderedTexture2D<uint> g_sc_wpe_rt : register(REG_U_SC_WPE_TO_ROV_SLOT);
RasterizerOrderedTexture2D<uint> g_sc_mirror_rt : register(REG_U_SC_WPE_MIRROR_SLOT);
#endif//__HLSL

#endif//SC_EFFECT_PASS_HLSLI