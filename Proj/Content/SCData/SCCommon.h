#pragma once
#include <Engine/Core/CoreMinimal.h>
#include <Engine/Core/Math.h>

#include <cmath>

namespace sc
{
	constexpr uint32_t Mask32(uint32_t bits)
	{
		return (1u << bits) - 1u;
	}

	// SC 좌표계 (+x right, +y down) -> World 좌표계 (+x right, +y up)
	inline constexpr float2 ScToWorld(float sc_x, float sc_y)
	{
		return float2(sc_x, -sc_y);
	}
	inline constexpr float2 ScIntToWorld(int32_t sc_x, int32_t sc_y)
	{
		return ScToWorld(static_cast<float>(sc_x), static_cast<float>(sc_y));
	}
	inline constexpr float2 WorldToSc(float world_x, float world_y)
	{
		return float2(world_x, -world_y);
	}
	inline int2 WorldToScInt(float world_x, float world_y)
	{
		return int2(static_cast<int32_t>(std::lround(world_x)), static_cast<int32_t>(std::lround(-world_y)));
	}
}