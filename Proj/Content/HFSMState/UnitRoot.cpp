#include "Content/pch.h"
#include "UnitRoot.h"

#include <Engine/Game/Component/BlackBoard.h>
#include <Engine/Game/Component/Transform.h>

#include <Engine/Core/Debug.h>

#include <Content/SCData/Constants.h>

#include <Content/HFSMState/SCBBCommon.h>

namespace sc
{
	UnitRoot::UnitRoot()
		: Super(UnitRoot::kClassConcreteName)
	{}

	UnitRoot::~UnitRoot()
	{}

	void UnitRoot::OnAwake(const engine::AIContext& ai_context)
	{
		using namespace engine;

		Super::OnAwake(ai_context);

		//BlackBoard에 필요한 초기값을 등록
		ASSERT(ai_context.black_board);
		BlackBoard* blackboard = ai_context.black_board;

		ASSERT(ai_context.game_object);
		ASSERT(ai_context.transform);
		Transform* tr = ai_context.transform;
		float2 pos = { tr->GetLocalPosition().x, tr->GetLocalPosition().y };
		
		sc::black_board::EnsureBlackBoardData(blackboard);

		float2* dest = blackboard->GetValue<float2>(sc::black_board::kDestination);
		ASSERT(dest != nullptr);
		*dest = pos;

		//sc는 256단계 선회각을 사용
		//회전의 최대 선회각 = 180 deg
		//선회 속도가 127일 경우 즉시 선회가 가능. (127/256 * 360 = 178.59375 deg)
		//sc의 선회 속도를 라디안으로 변환하면 다음과 같다
		float turn_speed = 40.0f / 256.0f * 2.0f * kPi * sc::kFPS;
		blackboard->SetValue(sc::black_board::kTurnSpeed, turn_speed);

		command_input_ = blackboard->GetValue<HashedStringView>(sc::black_board::kCommandInput);
		current_action_ = blackboard->GetValue<HashedStringView>(sc::black_board::kCurrentAction);
		ASSERT(command_input_);
		ASSERT(current_action_);
	}
	engine::HashedStringView UnitRoot::CheckTransition(const engine::AIContext& ai_context)
	{
		using namespace engine;

		//Consume command input
		HashedStringView entered_command = *command_input_;
		(*command_input_) = "";

		//다를 떄만 처리, 같으면 이미 진행 중이므로 무시
		if (entered_command != (*current_action_))
		{
			if (entered_command == "ForceMove")
			{
				// action 이미 ForceMove 상태일 경우 전환하지 않음(진행 중)
				if ((*current_action_) == "ForceMove")
				{
					return "";
				}

				*current_action_ = "ForceMove";
				return "Locomotion";
			}
		}

		return "";
	}
}

