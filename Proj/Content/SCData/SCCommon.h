#pragma once
#include <Engine/Core/CoreMinimal.h>
#include <Engine/Core/Math.h>

#include <SCSimulator/Include/SCSimulatorTypes.h>

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

	struct PlayerInputFrame
	{
		int2 mouse_sc = {};
		uint32_t hover_unit_id = SCSim::kInvalidUnitId;
		bool left_down = {};
		bool left_up = {};
		bool right_down = {};
		bool shift = {};
	};

	// 원작 클라이언트에서 클릭·드래그로 선택할 수 없는 유닛 타입
	constexpr bool IsSelectableUnitType(SCSim::UnitType unit_type)
	{
		switch (unit_type)
		{
		case SCSim::UnitType::TerranNuclearMissile:
		case SCSim::UnitType::ProtossScarab:
		case SCSim::UnitType::SpellDisruptionWeb:
		case SCSim::UnitType::SpellDarkSwarm:
		case SCSim::UnitType::SpecialUpperLevelDoor:
		case SCSim::UnitType::SpecialRightUpperLevelDoor:
		case SCSim::UnitType::SpecialPitDoor:
		case SCSim::UnitType::SpecialRightPitDoor:
			return false;
		default:
			return true;
		}
	}
}