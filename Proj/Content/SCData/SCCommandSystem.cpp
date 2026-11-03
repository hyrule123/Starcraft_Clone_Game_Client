#include "Content/pch.h"
#include "SCCommandSystem.h"

#include <Content/Scene/MainGameScene.h>

#include <SCSimulator/Include/SCSimulatorAPI.h>

namespace sc
{
	SCCommandSystem::SCCommandSystem(MainGameScene* owner_scene)
		: owner_scene_(owner_scene)
	{
		CHECK(owner_scene);
	}
	void SCCommandSystem::Update(const PlayerInputFrame& player_input)
	{
		if (player_input.right_down == false) { return; }

		const bool result = SCSim_RightClick(owner_scene_->GetSCSimContext(), owner_scene_->GetLocalPlayer(),
			player_input.mouse_sc.x, player_input.mouse_sc.y, player_input.hover_unit_id, SCSim::UnitType::None, player_input.shift);

		if (result == false)
		{
			DEBUG_LOG("SCSim_RightClick rejected: {}", SCSim_GetLastError());
		}
	}
}