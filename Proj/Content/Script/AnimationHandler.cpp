#include "Content/pch.h"
#include "AnimationHandler.h"

#include <Engine/Manager/TimeManager.h>

#include <Engine/Game/Component/Transform.h>
#include <Engine/Game/Component/SpriteAnimator.h>
#include <Engine/Game/Component/SpriteRenderer.h>
#include <Engine/Game/Component/BlackBoard.h>

#include <Engine/Core/Debug.h>

namespace engine
{
	AnimationHandler::AnimationHandler()
		: Super(AnimationHandler::kClassConcreteName)
		, current_anim_key_(""_hash)
	{}
	AnimationHandler::~AnimationHandler()
	{}

	void AnimationHandler::Awake()
	{
		Super::Awake();

		transform_ = GetOwnerGameObject()->GetComponent<Transform>();
		animator_ = GetOwnerGameObject()->GetComponent<SpriteAnimator>();
		renderer_ = GetOwnerGameObject()->GetComponent<SpriteRenderer>();
		blackboard_ = GetOwnerGameObject()->GetComponent<BlackBoard>();

		ASSERT(false == transform_.expired());
		ASSERT(false == animator_.expired());
		ASSERT(false == renderer_.expired());
		ASSERT(false == blackboard_.expired());

		s_ptr<BlackBoard> bb = blackboard_.lock();

		direction_ = bb->GetValue<float2>("Direction"_hash);
		ASSERT(direction_);

		playing_animation_ = bb->GetValue<HashedStringView>("PlayingAnimation"_hash);
		ASSERT(playing_animation_);

		ASSERT(false == anim_clips_table_.cont.empty());
		//VALITATION (모든 클립의 Direction 갯수는 같아야 함)
#ifndef NDEBUG
		auto iter = anim_clips_table_.cont.begin();
		for (iter; iter != anim_clips_table_.cont.end(); ++iter)
		{
			ASSERT_MESSAGE(direction_count_ == iter->second.size(), "모든 애니메이션 클립의 방향 갯수는 같아야 합니다.");
		}
#endif//NDEBUG
	}

	void AnimationHandler::Update()
	{
		Super::Update();

		s_ptr<Transform> tr = transform_.lock();
		ASSERT(tr);
		const float3 current_pos = tr->GetLocalPosition();

		bool is_dir_changed = false;
		bool is_anim_changed = false;

		float2 cur_dir = *direction_;

		//방향값이 변경되었을 경우에만 방향을 계산
		if (prev_direction_ != cur_dir)
		{
			is_dir_changed = true;

			//방향에 따른 재생 애니메이션 계산
			//Y축을 보고 있을 떄를 0도라고 가정 -> atan2f(x, y)로 계산하면 된다.
			float dir_rad = atan2f(cur_dir.x, cur_dir.y);
			needs_flip_lr_ = dir_rad < 0.0f;

			//음수값은 필요없으니 제거
			dir_rad = std::abs(dir_rad);

			//실제 animation index
			//0 ~ PI(180도) 를 0 ~ 1 사이로 줄이고 directions_count를 곱해준다.
			direction_idx_ = (size_t)std::round(dir_rad / kPi * (float)direction_count_);
			//혹시나 에러 방지
			direction_idx_ = std::clamp(direction_idx_, (size_t)0, direction_count_ - 1);

			
		}
		prev_direction_ = cur_dir;

		//애니메이션 변경 체크
		if (current_anim_key_ != *playing_animation_)
		{
			is_anim_changed = true;

			auto iter = anim_clips_table_.find(*playing_animation_);
			ASSERT_MESSAGE(iter != anim_clips_table_.end(), "그런 이름의 애니메이션은 없어요...");

			current_anim_key_ = *playing_animation_;
			current_anim_clip_ptr_ = &iter->second;
		}


		//애니메이션이 바뀌었을 때: 새로 플레이, Direction도 바뀌었을 수 있으므로 반영
		if (is_anim_changed)
		{
			animator_.lock()->Play((*current_anim_clip_ptr_)[direction_idx_]);
			renderer_.lock()->SetFlipLeftRight(needs_flip_lr_);
		}
		//애니메이션은 안 바뀌고 방향만 바뀌었을 때: 현재 프레임 유지한 채로 애니메이션 전환
		else if(is_dir_changed)
		{
			animator_.lock()->SwitchAnimKeepFrame((*current_anim_clip_ptr_)[direction_idx_]);
			renderer_.lock()->SetFlipLeftRight(needs_flip_lr_);
		}
		//이외: 아무것도 변경할 필요 없음


	}

	void AnimationHandler::SetDestination(const float2& destination)
	{
		destination_ = destination;
		is_moving_ = true;
	}
}