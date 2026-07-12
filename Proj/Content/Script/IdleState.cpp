#include "Content/pch.h"
#include "IdleState.h"

#include <Engine/Core/Debug.h>
#include <Engine/Core/Math.h>

#include <Engine/Manager/TimeManager.h>

#include <Engine/Game/Component/HFSM.h>
#include <Engine/Game/Component/Blackboard.h>

#include <Engine/Util/StringHashTable.h>

#include <array>

#include <Content/Component/SCSpriteAnimator.h>

namespace engine
{
	constexpr std::array <HashedStringView, 9> kIdleAnimNames = {
		"Idle_0"_hash,
		"Idle_1"_hash,
		"Idle_2"_hash,
		"Idle_3"_hash,
		"Idle_4"_hash,
		"Idle_5"_hash,
		"Idle_6"_hash,
		"Idle_7"_hash,
		"Idle_8"_hash,
	};

	constexpr std::array <HashedStringView, 9> kMoveAnimNames = {
	"Move_0"_hash,
	"Move_1"_hash,
	"Move_2"_hash,
	"Move_3"_hash,
	"Move_4"_hash,
	"Move_5"_hash,
	"Move_6"_hash,
	"Move_7"_hash,
	"Move_8"_hash,
	};

	IdleState::IdleState()
		: Super(IdleState::kClassConcreteName)
	{}
	IdleState::~IdleState()
	{}
	void IdleState::OnEnter()
	{
		Super::OnEnter();
		//IdleState에 진입했을 때 수행할 로직 작성
		DEBUG_LOG("IdleState::OnEnter() called.");

		animator_ = GetOwnerHFSM()->GetOwner()->GetComponent<SCSpriteAnimator>();
		ASSERT(!animator_.expired());

		animator_.lock()->Play("Idle_0"_hash);
	}
	void IdleState::OnExit()
	{
		Super::OnExit();

		DEBUG_LOG("IdleState::OnExit() called.");
	}
	void IdleState::OnUpdate()
	{
		Super::OnUpdate();

		acc_time_ += TimeManager::GetInst().DeltaTime();

		if (acc_time_ >= wait_time_)
		{
			acc_time_ -= wait_time_;

			cur_dir_idx_ = (cur_dir_idx_ + 1) % kIdleAnimNames.size();

			animator_.lock()->Play(kMoveAnimNames[cur_dir_idx_]);
		}

	}
	HashedStringView IdleState::CheckTransition()
	{
		float2* dest = GetOwnerHFSM()->GetBlackboard()->GetValue<float2>("Destination"_hash);

		if (dest)
		{
			return "Move"_hash;
		}


		return ""_hash;
	}
}