#include "Content/pch.h"
#include "Locomotion.h"

#include <Engine/Game/Component/BlackBoard.h>
#include <Engine/Game/Component/Transform.h>

#include <Engine/Manager/TimeManager.h>

#include <Engine/Core/Debug.h>

namespace sc
{
	Locomotion::Locomotion()
		: Super(Locomotion::kClassConcreteName)
	{}
	Locomotion::~Locomotion()
	{}
	void Locomotion::OnAwake(const engine::AIContext& ai_context)
	{
		Super::OnAwake(ai_context);

		ASSERT(ai_context.black_board);

		direction_ = ai_context.black_board->GetValue<float2>("Direction");
		ASSERT(direction_);

		destination_ = ai_context.black_board->GetValue<float2>("Destination");
		ASSERT(destination_);

		speed_ = ai_context.black_board->GetValue<float>("Speed");
		ASSERT(speed_);
	}
	void Locomotion::OnEnter(const engine::AIContext& ai_context)
	{
		Super::OnEnter(ai_context);
	}
	engine::HashedStringView Locomotion::CheckTransition(const engine::AIContext& ai_context)
	{
		ASSERT(ai_context.black_board);
		ASSERT(ai_context.transform);

		const float3& pos = ai_context.transform->GetLocalPosition();

		if (destination_->x != pos.x || destination_->y != pos.y)
		{
			return "Move";
		}
		else
		{
			return "Idle";
		}

		return "";
	}
}