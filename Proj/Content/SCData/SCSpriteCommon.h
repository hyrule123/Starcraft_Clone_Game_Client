#pragma once

#include <Engine/Core/CoreMinimal.h>
#include <Engine/Core/Debug.h>

#include <Content/HLSL/SCSpriteDebug.hlsli>

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
}
