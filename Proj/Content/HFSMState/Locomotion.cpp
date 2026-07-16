#include "Content/pch.h"
#include "Locomotion.h"

#include <Engine/Game/Component/BlackBoard.h>
#include <Engine/Game/Component/Transform.h>

#include <Engine/Manager/TimeManager.h>

#include <Engine/Core/Debug.h>

namespace engine
{
	Locomotion::Locomotion()
		: Super(Locomotion::kClassConcreteName)
	{}
	Locomotion::~Locomotion()
	{}
	void Locomotion::OnAwake(const AIContext& ai_context)
	{
		Super::OnAwake(ai_context);

		s_ptr<BlackBoard> bb = ai_context.black_board.lock();
		ASSERT(bb);

		direction_ = bb->GetValue<float2>("Direction"_hash);
		ASSERT(direction_);

		destination_ = bb->GetValue<float2>("Destination"_hash);
		ASSERT(destination_);

		speed_ = bb->GetValue<float>("Speed"_hash);
		ASSERT(speed_);
	}
	void Locomotion::OnEnter(const AIContext& ai_context)
	{
		Super::OnEnter(ai_context);
	}
	HashedStringView Locomotion::CheckTransition(const AIContext& ai_context)
	{
		ASSERT(ai_context.black_board.expired() == false);

		s_ptr<BlackBoard> bb = ai_context.black_board.lock();

		auto tr = ai_context.transform.lock();
		ASSERT(tr);

		const float3& pos = tr->GetLocalPosition();

		if (destination_->x != pos.x || destination_->y != pos.y)
		{
			return "Move"_hash;
		}
		else
		{
			return "Idle"_hash;
		}

		return ""_hash;
	}
}