#include "Content/pch.h"
#include "SCBBCommon.h"

#include <Engine/Game/Component/BlackBoard.h>

namespace sc::black_board
{
	void EnsureBlackBoardData(engine::BlackBoard* blackboard)
	{
		using namespace engine;

		bool* is_set = blackboard->GetValue<bool>(kIsSCDataSet);
		if (is_set == nullptr)
		{
			blackboard->SetValue(kIsSCDataSet, true);

			blackboard->SetValue(kCommandInput, HashedStringView(""));
			blackboard->SetValue(kCurrentAction, HashedStringView(""));
			blackboard->SetValue(kDestination, float2{ 0.0f, 0.0f });
			blackboard->SetValue(kDirection, float2{ 0.0f, -1.0f });
			blackboard->SetValue(kPlayingAnimation, HashedStringView(""));
			blackboard->SetValue(kSpeed, 50.0f);
			blackboard->SetValue(kIsMoving, false);
		}
	}
}