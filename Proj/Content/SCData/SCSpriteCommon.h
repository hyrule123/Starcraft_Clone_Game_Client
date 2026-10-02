#pragma once

#include <Engine/Core/CoreMinimal.h>
#include <Engine/Core/Debug.h>

#include <Content/HLSL/SCImage/SCImageCommon.hlsli>
#include <Content/HLSL/SCSpriteDebug.hlsli>

#include <cmath>

namespace sc
{
    constexpr uint32 SpriteFlagMask(uint32 bits)
    {
        return (1u << bits) - 1u;
    }

    inline uint32 PackSpriteFlags(bool flipped, uint32 player_color,
        uint32 modifier, uint32 modifier_data, uint32 lut_table)
    {
        ASSERT(player_color <= SpriteFlagMask(kSpriteColorBits));
        ASSERT(modifier <= SpriteFlagMask(kSpriteModifierBits));
        ASSERT(modifier_data <= SpriteFlagMask(kSpriteModifierDataBits));
        ASSERT(lut_table <= SpriteFlagMask(kSpriteLutBits));

        return ((flipped ? 1u : 0u) << kSpriteFlippedShift)
            | (player_color << kSpriteColorShift)
            | (modifier << kSpriteModifierShift)
            | (modifier_data << kSpriteModifierDataShift)
            | (lut_table << kSpriteLutShift);
    }

    constexpr uint32 SpriteFlagMaskDebug(uint32 bits)
    {
        return (1u << bits) - 1u;
    }

    inline uint32 PackSpriteFlagsDebug(bool flipped, uint32 player_color,

        uint32 modifier, uint32 modifier_data, uint32 lut_table)
    {
        ASSERT(player_color <= SpriteFlagMaskDebug(kSpriteColorBitsDebug));
        ASSERT(modifier <= SpriteFlagMaskDebug(kSpriteModifierBitsDebug));
        ASSERT(modifier_data <= SpriteFlagMaskDebug(kSpriteModifierDataBitsDebug));
        ASSERT(lut_table <= SpriteFlagMaskDebug(kSpriteLutBitsDebug));

        return ((flipped ? 1u : 0u) << kSpriteFlippedShiftDebug)
            | (player_color << kSpriteColorShiftDebug)
            | (modifier << kSpriteModifierShiftDebug)
            | (modifier_data << kSpriteModifierDataShiftDebug)
            | (lut_table << kSpriteLutShiftDebug);
    }

    constexpr float kSpriteDefaultZ = 0.5f;

	// SC 좌표계 (+x right, +y down) -> World 좌표계 (+x right, +y up)
    inline constexpr float2 ScToWorld(float sc_x, float sc_y)
    {
		return float2(sc_x, -sc_y);
    }
    inline constexpr float2 ScIntToWorld(int32 sc_x, int32 sc_y)
    {
		return ScToWorld(static_cast<float>(sc_x), static_cast<float>(sc_y));
    }
	inline constexpr float2 WorldToSc(float world_x, float world_y)
	{
		return float2(world_x, -world_y);
	}
	inline int2 WorldToScInt(float world_x, float world_y)
	{
		return int2(static_cast<int32>(std::lround(world_x)), static_cast<int32>(std::lround(-world_y)));
	}

	// Modifier 값에 따른 Material Variant
    enum class SCImageMaterialVariant
    {
        Normal,
        Shadow,
        kCount
    };
}
