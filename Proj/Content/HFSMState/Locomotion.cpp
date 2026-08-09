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

		ASSERT(ai_context.black_board);

		direction_ = ai_context.black_board->GetValue<float2>("Direction"_hash);
		ASSERT(direction_);

		destination_ = ai_context.black_board->GetValue<float2>("Destination"_hash);
		ASSERT(destination_);

		speed_ = ai_context.black_board->GetValue<float>("Speed"_hash);
		ASSERT(speed_);
	}
	void Locomotion::OnEnter(const AIContext& ai_context)
	{
		Super::OnEnter(ai_context);
	}
	HashedStringView Locomotion::CheckTransition(const AIContext& ai_context)
	{
		ASSERT(ai_context.black_board);
		ASSERT(ai_context.transform);

		const float3& pos = ai_context.transform->GetLocalPosition();

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