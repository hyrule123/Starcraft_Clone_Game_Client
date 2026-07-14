#include "Content/pch.h"
#include "UnitRoot.h"

#include <Engine/Game/Component/Blackboard.h>

#include <Engine/Core/Debug.h>

#include <Content/SCConstants/Command.h>

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

		//Blackboard에 필요한 초기값을 등록
		s_ptr<Blackboard> blackboard = ai_context.blackboard.lock();
		ASSERT(blackboard);

		blackboard->SetValue("Command"_hash, Command(std::monostate{}));
	}
}

