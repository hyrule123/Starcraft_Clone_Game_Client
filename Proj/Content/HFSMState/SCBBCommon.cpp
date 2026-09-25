#include "Content/pch.h"
#include "SCBBCommon.h"

#include <Engine/Game/Component/BlackBoard.h>

namespace engine
{
	namespace SC::black_board
	{
		void EnsureBlackBoardData(BlackBoard* blackboard)
		{
			bool* is_set = blackboard->GetValue<bool>(kIsSCDataSet);
			if (is_set == nullptr)
			{
				blackboard->SetValue(kIsSCDataSet, true);

				blackboard->SetValue(kCommandInput, HashedStringView(""_hash));
				blackboard->SetValue(kCurrentAction, HashedStringView(""_hash));
				blackboard->SetValue(kDestination, float2{ 0.0f, 0.0f });
				blackboard->SetValue(kDirection, float2{ 0.0f, -1.0f });
				blackboard->SetValue(kPlayingAnimation, HashedStringView(""_hash));
				blackboard->SetValue(kSpeed, 50.0f);
				blackboard->SetValue(kIsMoving, false);
			}
		}
	}
}