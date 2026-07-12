#include "Content/pch.h"
#include "MoveState.h"

#include <Engine/Core/Debug.h>

namespace engine
{
	MoveState::MoveState()
		: Super(MoveState::kClassConcreteName)
	{}
	MoveState::~MoveState()
	{}
	void MoveState::OnEnter()
	{
		Super::OnEnter();

		DEBUG_LOG("MoveState::OnEnter() called.");
	}
	void MoveState::OnExit()
	{
		Super::OnExit();

		DEBUG_LOG("MoveState::OnExit() called.");
	}
}