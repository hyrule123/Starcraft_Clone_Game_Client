#pragma once
#include <Content/SCData/SCCommon.h>

namespace sc
{
	class MainGameScene;

	class SCCommandSystem
	{
	public:
		explicit SCCommandSystem(MainGameScene* owner_scene);

		void Update(const PlayerInputFrame& player_input);	// 우클릭 → SCSim_RightClick (Shift = queue)

	private:
		MainGameScene* owner_scene_ = {};
	};
}


