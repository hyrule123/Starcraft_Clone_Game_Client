#pragma once
#include <Engine/Game/Component/HFSMState.h>

#include <Engine/Core/CoreMinimal.h>

namespace engine
{
	class SpriteAnimator;
}

namespace sc
{
    class Idle :
        public engine::HFSMState
    {
		ENTITY_INFO(Idle, engine::HFSMState)
    public:
		Idle();
		virtual ~Idle() override;

		virtual void OnAwake(const engine::AIContext& ai_context) override;
		virtual void OnEnter(const engine::AIContext& ai_context) override;
		virtual void OnExit(const engine::AIContext& ai_context) override;
		virtual void OnUpdate(const engine::AIContext& ai_context) override;

		virtual engine::HashedStringView CheckTransition(const engine::AIContext& ai_context) override;

    private:
		engine::HashedStringView* playing_animation_ = {};
    };
}

