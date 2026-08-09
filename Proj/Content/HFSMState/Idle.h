#pragma once
#include <Engine/Game/Component/HFSMState.h>

#include <Engine/Core/CoreMinimal.h>



namespace engine
{
	class SpriteAnimator;

    class Idle :
        public HFSMState
    {
		ENTITY_INFO(Idle, HFSMState)
    public:
		Idle();
		virtual ~Idle() override;

		virtual void OnAwake(const AIContext& ai_context) override;
		virtual void OnEnter(const AIContext& ai_context) override;
		virtual void OnExit(const AIContext& ai_context) override;
		virtual void OnUpdate(const AIContext& ai_context) override;

		virtual HashedStringView CheckTransition(const AIContext& ai_context) override;

    private:
		HashedStringView* playing_animation_ = {};
    };
}

