#include "Content/pch.h"
#include "MovementController.h"

#include <Engine/Manager/TimeManager.h>

#include <Engine/Game/Component/Transform.h>
#include <Engine/Game/Component/SpriteAnimator.h>
#include <Engine/Game/Component/SpriteRenderer.h>

#include <Engine/Core/Debug.h>

namespace engine
{
	MovementController::MovementController()
		: Super(MovementController::kClassConcreteName)
		, direction_(float2(0.0f, -1.0f)) //아래쪽을 바라보는 방향
	{}
	MovementController::~MovementController()
	{}

	void MovementController::Awake()
	{
		Super::Awake();

		transform_ = GetOwner()->GetComponent<Transform>();
		animator_ = GetOwner()->GetComponent<SpriteAnimator>();
		renderer_ = GetOwner()->GetComponent<SpriteRenderer>();
		ASSERT(!transform_.expired());
		ASSERT(!animator_.expired());
		ASSERT(!renderer_.expired());
	}

	void MovementController::Update()
	{
		Super::Update();
		if (is_moving_)
		{
			TimeManager& time_mgr = TimeManager::GetInst();
			float dt = time_mgr.DeltaTime();

			s_ptr<Transform> tr = transform_.lock();

			float3 current_pos = tr->GetLocalPosition();
			float distance = (destination_ - float2(current_pos.x, current_pos.y)).Length();

			//도착 확인
			float dist_threshold = (speed_ * dt) * 1.2f;
			if (distance <= dist_threshold)
			{
				tr->SetLocalPosition(float3(destination_, current_pos.z));
				is_moving_ = false;


			}
			else
			{
				float2 new_pos = float2(current_pos.x, current_pos.y) + direction_ * speed_ * dt;
				tr->SetLocalPosition(float3(new_pos, current_pos.z));
			}
		}
	}

	void MovementController::SetDestination(const float2& destination)
	{
		destination_ = destination;

		s_ptr<Transform> tr = transform_.lock();
		float3 current_pos = tr->GetLocalPosition();

		direction_ = destination_ - float2(current_pos.x, current_pos.y);
		direction_.Normalize();

		//방향에 따른 재생 애니메이션 계산
		//Y축을 보고 있을 떄를 0도라고 가정 -> atan2f(x, y)로 계산하면 된다.
		float dir_rad = atan2f(direction_.x, direction_.y);
		bool is_flip_lr = dir_rad < 0.0f;

		//음수값은 필요없으니 제거
		dir_rad = std::abs(dir_rad);

		//실제 animation index
		//0 ~ PI(180도) 를 0 ~ 1 사이로 줄이고 directions_count를 곱해준다.
		size_t dir_idx = (size_t)std::round(dir_rad / kPi * (float)move_clips_.size());
		//혹시나 에러 방지
		dir_idx = std::clamp(dir_idx, (size_t)0, move_clips_.size() - 1);

		s_ptr<SpriteAnimator> animator = animator_.lock();
		ASSERT(animator);

		// 이제 이동을 시작한거면 Play()를 호출하고, 
		if (!is_moving_)
		{
			animator->Play(move_clips_[dir_idx]);
		}
		//이미 이동 중이라면 SwitchAnimKeepFrame()을 호출하여 현재 프레임을 유지한 채로 애니메이션을 전환한다.
		else
		{
			animator->SwitchAnimKeepFrame(move_clips_[dir_idx]);
		}
		
		s_ptr<SpriteRenderer> renderer = renderer_.lock();
		ASSERT(renderer);
		renderer->SetFlipLeftRight(is_flip_lr);

		is_moving_ = true;
	}

}