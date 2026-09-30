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
	class BlackBoard;
}

namespace sc
{

	//방향에 따른 Animation Clip을 출력하기 위한 중간 클래스
	//애니메이션 재생에 필요한 것: '어떤 애니메이션'을 '어떤 방향'으로 출력할 것인가
	//그거 외의 정보는 참조하지 않음
    class AnimationHandler :
        public engine::Script
    {
        ENTITY_INFO(AnimationHandler, engine::Script)
	public:
		AnimationHandler();
		virtual ~AnimationHandler() override;

		virtual void Awake() override;
		virtual void Update() override;

		void SetDestination(const float2& destination);

		void SetMoveClips(const engine::HashedStringView& key, const std::vector<engine::SpriteAnimClip*>& move_clips) { 
			anim_clips_table_.insert(key, move_clips);
			direction_count_ = move_clips.size();
		}

		//현재 재생중인 Animation과 다를 경우에만
		void PlayAnimation(const engine::HashedStringView& key);

    private:
		engine::Transform* transform_ = {};
		engine::SpriteAnimator* animator_ = {};
		engine::SpriteRenderer* renderer_ = {};
		engine::BlackBoard* blackboard_ = {};

		size_t direction_count_ = {};
		
		float2 prev_direction_ = {};
		float2* direction_ = {};
		size_t direction_idx_ = {};
		bool needs_flip_lr_ = {};

		engine::HashedStringView* playing_animation_ = {};
		engine::HashedStringView current_anim_key_;
		std::vector<engine::SpriteAnimClip*>* current_anim_clip_ptr_ = {};

		engine::StringHashTable<std::vector<engine::SpriteAnimClip*>> anim_clips_table_ = {};

		float2 destination_ = {};
		
		float speed_ = 400.0f;

		bool is_moving_ = {};
    };
}


