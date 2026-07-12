#pragma once
#include <Engine/Game/Component/HFSMState.h>

#include <Engine/Core/CoreMinimal.h>

namespace engine
{
	class SpriteAnimator;

    class IdleState :
        public HFSMState
    {
		CLASS_INFO(IdleState, HFSMState)
    public:
		IdleState();
		virtual ~IdleState() override;

		virtual void OnEnter() override;
		virtual void OnUpdate() override;

		virtual HashedStringView CheckTransition() override;

    private:
        constexpr inline static float wait_time_ = 3.0f;
		float acc_time_ = 0.0f;
		uint32 cur_dir_idx_ = 0u;

		w_ptr<SpriteAnimator> animator_ = {};
    };
}

