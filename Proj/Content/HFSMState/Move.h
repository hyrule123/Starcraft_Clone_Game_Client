#pragma once
#include <Engine/Game/Component/HFSMState.h>

#include <Engine/Core/Math.h>

namespace engine
{
	//class HashedStringView;
	class AnimationHandler;

    class Move :
        public HFSMState
    {
		CLASS_INFO(Move, HFSMState)
	public:
		Move();
		virtual ~Move() override;
		
		virtual void OnAwake(const AIContext& ai_context) override;
		virtual void OnEnter(const AIContext& ai_context) override;
		virtual void OnUpdate(const AIContext& ai_context) override;
		virtual void OnExit(const AIContext& ai_context) override;

	private:
		HashedStringView* playing_animation_ = {};
		bool* is_moving_ = {};
		float2* cur_direction_ = {};
		float2* destination_ = {};
		float* speed_ = {};
		float* turn_speed_ = {};
    };
}


