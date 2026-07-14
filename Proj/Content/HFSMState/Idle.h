#pragma once
#include <Engine/Game/Component/HFSMState.h>

#include <Engine/Core/CoreMinimal.h>

namespace engine
{
	class SpriteAnimator;

    class Idle :
        public HFSMState
    {
		CLASS_INFO(Idle, HFSMState)
    public:
		Idle();
		virtual ~Idle() override;

		virtual void OnEnter(const AIContext& ai_context) override;
		virtual void OnExit(const AIContext& ai_context) override;
		virtual void OnUpdate(const AIContext& ai_context) override;

		virtual HashedStringView CheckTransition(const AIContext& ai_context) override;

    private:
        constexpr inline static float wait_time_ = 3.0f;
		float acc_time_ = 0.0f;
		uint32 cur_dir_idx_ = 0u;

		w_ptr<SpriteAnimator> animator_ = {};
    };
}

