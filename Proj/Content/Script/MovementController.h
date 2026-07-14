#pragma once
#include <Engine/Game/Component/Script.h>

#include <Engine/Core/CoreMinimal.h>

#include <Engine/Core/Math.h>

namespace engine
{
	class Transform;
	class SpriteAnimator;
	class SpriteRenderer;
	class SpriteAnimClip;

    class MovementController :
        public Script
    {
        CLASS_INFO(MovementController, Script)
	public:
		MovementController();
		virtual ~MovementController() override;

		virtual void Awake() override;
		virtual void Update() override;

		void SetDestination(const float2& destination);

		void SetMoveClips(const std::vector<SpriteAnimClip*>& move_clips) { 
			move_clips_ = move_clips; 
		}

    private:
		w_ptr<Transform> transform_ = {};
		w_ptr<SpriteAnimator> animator_ = {};
		w_ptr<SpriteRenderer> renderer_ = {};

		//각 방향별 이동 애니메이션 클립(위쪽 보는것부터 각도순 / 180도)
		std::vector<SpriteAnimClip*> move_clips_ = {};
		size_t direction_idx_ = {};

		float2 destination_ = {};
		float2 direction_;
		float speed_ = 400.0f;

		bool is_moving_ = {};
    };
}


