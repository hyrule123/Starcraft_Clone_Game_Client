#include "Content/pch.h"
#include "UnitRoot.h"

#include <Engine/Game/Component/BlackBoard.h>
#include <Engine/Game/Component/Transform.h>

#include <Engine/Core/Debug.h>

#include <Content/SCData/Constant.h>

namespace engine
{
	UnitRoot::UnitRoot()
		: Super(UnitRoot::kClassConcreteName)
	{}

	UnitRoot::~UnitRoot()
	{}

	void UnitRoot::OnAwake(const AIContext& ai_context)
	{
		Super::OnAwake(ai_context);

		//BlackBoard에 필요한 초기값을 등록
		ASSERT(ai_context.black_board);
		BlackBoard* blackboard = ai_context.black_board;

		ASSERT(ai_context.game_object);
		ASSERT(ai_context.transform);
		Transform* tr = ai_context.transform;
		float2 pos = { tr->GetLocalPosition().x, tr->GetLocalPosition().y };

		blackboard->SetValue("CommandInput"_hash, HashedStringView(""_hash));
		blackboard->SetValue("CurrentAction"_hash, HashedStringView(""_hash));
		blackboard->SetValue("Destination"_hash, pos);
		blackboard->SetValue("Direction"_hash, float2{0.0f, -1.0f});
		blackboard->SetValue("PlayingAnimation"_hash, HashedStringView(""_hash));
		blackboard->SetValue("Speed"_hash, 100.0f);
		blackboard->SetValue("IsMoving"_hash, false);

		//SC는 256단계 선회각을 사용
		//회전의 최대 선회각 = 180 deg
		//선회 속도가 127일 경우 즉시 선회가 가능. (127/256 * 360 = 178.59375 deg)
		//SC의 선회 속도를 라디안으로 변환하면 다음과 같다
		float turn_speed = 40.0f / 256.0f * 2.0f * kPi * SC::kFPS;
		blackboard->SetValue("TurnSpeed"_hash, turn_speed);

		command_input_ = blackboard->GetValue<HashedStringView>("CommandInput"_hash);
		current_action_ = blackboard->GetValue<HashedStringView>("CurrentAction"_hash);
		ASSERT(command_input_);
		ASSERT(current_action_);
	}
	HashedStringView UnitRoot::CheckTransition(const AIContext& ai_context)
	{
		//Consume command input
		HashedStringView entered_command = *command_input_;
		(*command_input_) = ""_hash;

		//다를 떄만 처리, 같으면 이미 진행 중이므로 무시
		if (entered_command != (*current_action_))
		{
			if (entered_command == "ForceMove"_hash)
			{
				// action 이미 ForceMove 상태일 경우 전환하지 않음(진행 중)
				if ((*current_action_) == "ForceMove"_hash)
				{
					return ""_hash;
				}

				*current_action_ = "ForceMove"_hash;
				return "Locomotion"_hash;
			}
		}

		return ""_hash;
	}
}

