#pragma once

#include <Engine/Core/CoreMinimal.h>
#include <Engine/Core/Debug.h>

#include <SCSimulator/Include/SCSimulatorTypes.h>

#include <Content/HLSL/SCImage/SCImageCommon.hlsli>

#include <cmath>

namespace SCSim
{
    struct SpriteData;
}

namespace sc
{
    constexpr uint32_t Mask32(uint32_t bits)
    {
        return (1u << bits) - 1u;
    }

    inline uint32 PackSpriteFlags(bool flipped, uint32 player_color,
        uint32 modifier, uint32 modifier_data, uint32 lut_table)
    {
        ASSERT(player_color <= Mask32(kSpriteColorBits));
        ASSERT(modifier <= Mask32(kSpriteModifierBits));
        ASSERT(modifier_data <= Mask32(kSpriteModifierDataBits));
        ASSERT(lut_table <= Mask32(kSpriteLutBits));

        return ((flipped ? 1u : 0u) << kSpriteFlippedShift)
            | (player_color << kSpriteColorShift)
            | (modifier << kSpriteModifierShift)
            | (modifier_data << kSpriteModifierDataShift)
            | (lut_table << kSpriteLutShift);
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

    // SyncSprites가 이미지마다 1회 구성해 전달하는 값
    struct SCImageParam
    {
        // SCImageCommon.hlsli의 depth_key에 그대로 사용하는 변수. 설명은 해당 항목의 주석 참고
        uint32_t depth_key = 0;

        // 플레이어 색 번호 (0~15). owner가 아니라 players[owner].color 값.
        // GRP 원본 인덱스 8~15를 tunit[player_color_idx * 8 + (index - 8)]로 치환하는 데 사용.
        uint32_t player_color_idx = 0;
    };
}
