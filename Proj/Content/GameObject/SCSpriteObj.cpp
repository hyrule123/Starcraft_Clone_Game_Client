#include "Content/pch.h"
#include "SCSpriteObj.h"

#include <Engine/Core/Debug.h>

#include <Engine/Game/Component/Transform.h>

#include <SCSimulator/Include/SCSimulatorAPI.h>

#include <Content/SCData/Image/SCSpriteCommon.h>

#include <limits>

namespace sc
{
	SCSpriteObj::SCSpriteObj()
		: Super(SCSpriteObj::kClassConcreteName)
	{}

	SCSpriteObj::~SCSpriteObj()
	{}
	void SCSpriteObj::ApplySnapshot(const SCSim::SpriteData& sprite_data)
	{
		// SCSpriteObj는 루트를 기준으로 함
		const float3 world_pos_new = { ScIntToWorld(sprite_data.x, sprite_data.y), kSpriteDefaultZ };
		engine::Transform* transform = GetTransform();
		const float3 world_pos_old = transform->GetLocalPosition();

		// 정수 기준이고, 범위가 8192까지 이므로 직접 비교해도 무방
		if (world_pos_new != world_pos_old)
		{
			transform->SetLocalPosition(world_pos_new);
		}
	}
}

