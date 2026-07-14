#include "Content/pch.h"
#include "Move.h"

#include <Engine/Core/Debug.h>

#include <Engine/Game/Component/Blackboard.h>


#include <Content/Script/MovementController.h>

namespace engine
{
	Move::Move()
		: Super(Move::kClassConcreteName)
	{}
	Move::~Move()
	{}
	void Move::OnAwake(const AIContext& ai_context)
	{
		Super::OnAwake(ai_context);

		s_ptr<GameObject> obj = ai_context.owner.lock();
		ASSERT(obj);

		movement_controller_ = obj->GetComponent<MovementController>();
		ASSERT(movement_controller_.expired() == false);

		//아예 command의 주소를 받아 놓는다
		s_ptr<Blackboard> blackboard = ai_context.blackboard.lock();
		ASSERT(blackboard);
		command_ = blackboard->GetValue<Command>("Command"_hash);
		ASSERT(command_);
	}
	void Move::OnUpdate(const AIContext& ai_context)
	{
		Super::OnUpdate(ai_context);

		auto* movecmd = std::get_if<MoveCommand>(command_);
		if (movecmd)
		{
			s_ptr<MovementController> movement_controller = movement_controller_.lock();
			ASSERT(movement_controller);

			//목적지 갱신
			movement_controller->SetDestination(movecmd->destination);

			//원상 복구
			(*command_) = Command(std::monostate{});
		}
	}
	void Move::OnExit(const AIContext& ai_context)
	{
		Super::OnExit(ai_context);
		DEBUG_LOG("Move::OnExit() called.");
	}
}