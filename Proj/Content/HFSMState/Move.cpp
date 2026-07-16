#include "Content/pch.h"
#include "Move.h"

#include <Engine/Core/Debug.h>

#include <Engine/Manager/TimeManager.h>

#include <Engine/Game/Component/BlackBoard.h>
#include <Engine/Game/Component/Transform.h>


#include <Content/Script/AnimationHandler.h>

namespace engine
{
	Move::Move()
		: Super(Move::kClassConcreteName)
	{}
	Move::~Move()
	{}

	void Move::OnAwake(const AIContext& ai_context)
	{
		Super::OnAwake(ai_context);

		s_ptr<GameObject> obj = ai_context.game_object.lock();
		ASSERT(obj);

		s_ptr<BlackBoard> blackboard = ai_context.black_board.lock();
		ASSERT(blackboard);

		playing_animation_ = blackboard->GetValue<HashedStringView>("PlayingAnimation"_hash);
		ASSERT(playing_animation_);

		is_moving_ = blackboard->GetValue<bool>("IsMoving"_hash);
		ASSERT(is_moving_);

		cur_direction_ = blackboard->GetValue<float2>("Direction"_hash);
		ASSERT(cur_direction_);

		destination_ = blackboard->GetValue<float2>("Destination"_hash);
		ASSERT(destination_);

		speed_ = blackboard->GetValue<float>("Speed"_hash);
		ASSERT(speed_);

		turn_speed_ = blackboard->GetValue<float>("TurnSpeed"_hash);
		ASSERT(turn_speed_);
	}

	void Move::OnEnter(const AIContext& ai_context)
	{
		Super::OnEnter(ai_context);

		*playing_animation_ = "Move"_hash;
		*is_moving_ = true;
	}

	void Move::OnUpdate(const AIContext& ai_context)
	{
		Super::OnUpdate(ai_context);

		s_ptr<Transform> tr = ai_context.transform.lock();
		ASSERT(tr);
		float3 cur_pos = tr->GetLocalPosition();

		*is_moving_ = (cur_pos.x != destination_->x || cur_pos.y != destination_->y);

		if (*is_moving_)
		{
			float dt = TimeManager::GetInst().DeltaTime();

			//현재 위치 받고
			const float2 cur_pos_2d = { cur_pos.x, cur_pos.y };

			//이동 방향 확인
			float2 desired_direction = *destination_ - cur_pos_2d;
			desired_direction.Normalize();

			//현재 방향과의 차이 비교
			float dot = cur_direction_->Dot(desired_direction);	//내적 결과 = cos(theta)
			float cross = cur_direction_->Cross(desired_direction).x;	//외적 결과 = sin(theta) (z축 기준)

			//atan2 -> rad with direction
			float diff_rad = std::atan2(cross, dot);	//atan2(sin, cos) = rad

			float turn_speed_dt = *turn_speed_ * dt;
			bool needs_turn = std::abs(diff_rad) > turn_speed_dt;

			//방향 회전
			if (needs_turn)
			{
				//부호 가져옴
				float turn_angle = diff_rad > 0.0f ? turn_speed_dt : -turn_speed_dt;

				float cos_theta = std::cos(turn_angle);
				float sin_theta = std::sin(turn_angle);

				float2 new_direction;
				new_direction.x = (cur_direction_->x * cos_theta - cur_direction_->y * sin_theta);
				new_direction.y = (cur_direction_->x * sin_theta + cur_direction_->y * cos_theta);
				new_direction.Normalize();
				
				*cur_direction_ = new_direction;
			}
			else
			{
				*cur_direction_ = desired_direction;
			}

			//방향 일치 후 전진
			if (!needs_turn)
			{
				float move_dist = *speed_ * dt;
				float move_dist_sq = move_dist * move_dist;
				float dist_left = float2::DistanceSquared(cur_pos_2d, *destination_);

				//이번에 움직일 수 있는 거리가 남은 거리보다 작으면 이동, 
				if (dist_left >= move_dist_sq)
				{
					float2 new_pos = cur_pos_2d + (*cur_direction_) * move_dist;
					tr->SetLocalPosition(float3(new_pos, cur_pos.z));
				}
				//아니면 목적지 값으로 정확히 설정 후 종료
				else
				{
					tr->SetLocalPosition(float3(*destination_, cur_pos.z));
					*is_moving_ = false;
				}
			}
		}
	}
	void Move::OnExit(const AIContext& ai_context)
	{
		Super::OnExit(ai_context);
		DEBUG_LOG("Move::OnExit() called.");
		*is_moving_ = false;
	}
}