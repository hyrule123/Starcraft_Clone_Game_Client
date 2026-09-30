#pragma once
#include <Engine/Game/Component/HFSMState.h>

#include <Engine/Core/Math.h>

namespace sc
{
	//class HashedStringView;
	class AnimationHandler;

    class Move :
        public engine::HFSMState
    {
		ENTITY_INFO(Move, engine::HFSMState)
	public:
		Move();
		virtual ~Move() override;
		
		virtual void OnAwake(const engine::AIContext& ai_context) override;
		virtual void OnEnter(const engine::AIContext& ai_context) override;
		virtual void OnUpdate(const engine::AIContext& ai_context) override;
		virtual void OnExit(const engine::AIContext& ai_context) override;

	private:
		engine::HashedStringView* playing_animation_ = {};
		bool* is_moving_ = {};
		float2* cur_direction_ = {};
		float2* destination_ = {};
		float* speed_ = {};
		float* turn_speed_ = {};
    };
}


