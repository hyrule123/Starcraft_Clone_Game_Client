#pragma once
#include <Engine/Game/Component/HFSMState.h>

#include <Content/SCConstants/Command.h>

namespace engine
{
	class MovementController;

    class Move :
        public HFSMState
    {
		CLASS_INFO(Move, HFSMState)
	public:
		Move();
		virtual ~Move() override;

		virtual void OnAwake(const AIContext& ai_context) override;
		virtual void OnUpdate(const AIContext& ai_context) override;
		virtual void OnExit(const AIContext& ai_context) override;

	private:
		w_ptr<MovementController> movement_controller_ = {};
		Command* command_ = {};
    };
}


