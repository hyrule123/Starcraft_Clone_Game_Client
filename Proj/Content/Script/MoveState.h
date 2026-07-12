#pragma once
#include <Engine/Game/Component/HFSMState.h>

namespace engine
{
    class MoveState :
        public HFSMState
    {
		CLASS_INFO(MoveState, HFSMState)
	public:
		MoveState();
		virtual ~MoveState() override;

		virtual void OnEnter() override;
		virtual void OnExit() override;
    };
}


