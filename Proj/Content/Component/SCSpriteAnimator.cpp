#include "Content/pch.h"
#include "SCSpriteAnimator.h"

#include <Engine/Manager/ResourceManager.h>
#include <Engine/Manager/TimeManager.h>

#include <Engine/Resource/SpriteAnimation.h>
#include <Engine/Resource/SpriteAnimClip.h>

#include <Content/Component/SCSpriteRenderer.h>

#include <Engine/Core/Debug.h>

namespace engine
{
	SCSpriteAnimator::SCSpriteAnimator()
		: Super(SCSpriteAnimator::kClassConcreteName, SCSpriteAnimator::kComponentCategory)
	{}
	SCSpriteAnimator::~SCSpriteAnimator()
	{}
	void SCSpriteAnimator::Init()
	{
		Super::Init();
	}
	void SCSpriteAnimator::Awake()
	{
		Super::Awake();
		s_ptr<SCSpriteRenderer> sprite_renderer = GetComponent<SCSpriteRenderer>();
		ASSERT_MESSAGE(sprite_renderer, "SCSpriteRenderer가 존재하지 않습니다.");
		renderer_ = sprite_renderer;

		ASSERT_MESSAGE(anim_, "Sprite Animation이 존재하지 않습니다.");
		ASSERT_MESSAGE(anim_->IsReady(), "Sprite Animation이 준비되지 않았습니다.");
	}
	void SCSpriteAnimator::LateUpdate()
	{
		Super::LateUpdate();

		//Animation Clip 있고, 재생 중이라면 로직 처리
		if (playing_clip_ && is_playing_)
		{
			// 시간 누적
			acc_deltatime_ += TimeManager::GetInst().DeltaTime();

			//while문을 사용하여
			while (acc_deltatime_ >= cur_frame_duration_)
			{
				// 시간 차감
				acc_deltatime_ -= cur_frame_duration_;

				// 다음 프레임으로 이동
				cur_frame_idx_++;

				// 애니메이션의 끝(마지막 프레임 초과)에 도달했을 때의 처리
				if (cur_frame_idx_ >= clip_frame_total_count_)
				{
					if (playing_clip_->IsLoop())
					{
						// 루프 진행
						cur_frame_idx_ = 0u;
					}
					else
					{
						// Non-loop: 마지막 프레임에 고정
						cur_frame_idx_ = (uint32)(playing_clip_->GetFrames().size() - 1);

						// 더 이상 시간이 누적되어 오버플로우가 발생하지 않도록 시간 고정
						acc_deltatime_ = playing_clip_->GetFrames()[cur_frame_idx_].duration;

						//스위치 OFF
						is_playing_ = false;

						break;
					}
				}

				// 다음 순회(Iteration)를 위해 다음 프레임의 duration으로 갱신
				cur_frame_duration_ = playing_clip_->GetFrames()[cur_frame_idx_].duration;
			}

			//참조해야 하는 TextureArray의 Index를 전달
			if (!renderer_.expired())
			{
				renderer_.lock()->SetFrameIndex(playing_clip_->GetFrames()[cur_frame_idx_].index);
			}
		}
	}
	bool SCSpriteAnimator::SetSpriteAnimation(const HashedStringView& res_path)
	{
		anim_ = ResourceManager::GetInst().Find<SpriteAnimation>(res_path);
		return (bool)anim_;
	}
	bool SCSpriteAnimator::Play(const HashedStringView& anim_name)
	{
		if (!anim_) { return false; }
		SpriteAnimClip* clip = anim_->GetAnimationClip(anim_name);
		if (clip)
		{
			is_playing_ = true;
			acc_deltatime_ = 0.0f;
			cur_frame_idx_ = 0u;
			cur_frame_duration_ = clip->GetFrames()[0].duration;
			clip_frame_total_count_ = (uint32)clip->GetFrames().size();

			playing_clip_ = clip;
			return true;
		}

		return false;
	}
}