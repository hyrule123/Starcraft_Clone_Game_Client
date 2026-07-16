#include "Content/pch.h"
#include "UnitRoot.h"

#include <Engine/Game/Component/BlackBoard.h>
#include <Engine/Game/Component/Transform.h>

#include <Engine/Core/Debug.h>

#include <Content/SCConstants/Constant.h>

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
		s_ptr<BlackBoard> blackboard = ai_context.black_board.lock();
		ASSERT(blackboard);

		s_ptr<Transform> tr = ai_context.game_object.lock()->GetTransform();
		ASSERT(tr);
		float2 pos = { tr->GetLocalPosition().x, tr->GetLocalPosition().y };

		blackboard->SetValue("CommandInput"_hash, HashedStringView(""_hash));
		blackboard->SetValue("CurrentAction"_hash, HashedStringView(""_hash));
		blackboard->SetValue("Destination"_hash, pos);
		blackboard->SetValue("Direction"_hash, float2{0.0f, -1.0f});
		blackboard->SetValue("PlayingAnimation"_hash, HashedStringView(""_hash));
		blackboard->SetValue("Speed"_hash, 100.0f);
		blackboard->SetValue("IsMoving"_hash, false);

		float turn_speed = SC::DegreePerSecond(40.0f);
		blackboard->SetValue("TurnSpeed"_hash, turn_speed);

		command_input_ = blackboard->GetValue<HashedStringView>("CommandInput"_hash);
		current_action_ = blackboard->GetValue<HashedStringView>("CurrentAction"_hash);
		ASSERT(command_input_);
		ASSERT(current_action_);
	}
	HashedStringView UnitRoot::CheckTransition(const AIContext& ai_context)
	{
		if ((*command_input_) == "ForceMove"_hash)
		{
			*command_input_ = ""_hash;
			*current_action_ = "ForceMove"_hash;
			return "Locomotion"_hash;
		}

		return ""_hash;
	}
}

