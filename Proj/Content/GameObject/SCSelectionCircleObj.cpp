#include "Content/pch.h"
#include "SCSelectionCircleObj.h"

#include <Content/Component/SCImageRenderer.h>

#include <Content/SCData/SCResourceManager.h>

#include <Content/SCData/SCSpriteCommon.h>

#include <Engine/Game/Component/Transform.h>

namespace sc
{
	SCSelectionCircleObj::SCSelectionCircleObj()
		: Super(SCSelectionCircleObj::kClassConcreteName)
	{}
	SCSelectionCircleObj::~SCSelectionCircleObj()
	{}
	void SCSelectionCircleObj::Init()
	{
		Super::Init();

		renderer_ = AddComponent<SCImageRenderer>();
		renderer_->SetMesh(SCResourceManager::GetInst().GetImageMesh());
	}
	void SCSelectionCircleObj::ApplySelection(int32_t sprite_x, int32_t sprite_y, const SCSim::SpriteTypeData& sprite_type_data, uint32_t depth_key, uint32_t color_group)
	{
		// 참고: 이 오브젝트는 부모 SpriteObj와 명시적으로 붙지 않음(NO Transform->AddChild()), 루트 오브젝트 유지
		// 현재 구조 상, 명시적으로 붙이면 Destroy 시 삭제될수 있기 때문
		// 대신, 부모격인 대상 스프라이트 좌표에 Circle 오프셋 위치를 계산하여 LocalPosition을 설정함.
		const float2 pos = ScIntToWorld(sprite_x, sprite_y + sprite_type_data.selection_circle_vpos);	// 원작: sprite->position + (0, vpos)
		const float3 circle_world_pos = { pos.x, pos.y, kSpriteDefaultZ };	// 루트 오브젝트라 로컬 = 월드
		engine::Transform* transform = GetTransform();
		if (transform->GetLocalPosition() != circle_world_pos)
		{
			transform->SetLocalPosition(circle_world_pos);
		}

		ASSERT(0 <= sprite_type_data.selection_circle && sprite_type_data.selection_circle < SCSim::kSelectionCircleSizeCount);
		SCSim::ImageData img_data = {};
		img_data.image_type = SCSim::kImageTypeSelectionCircleBase + sprite_type_data.selection_circle;
		img_data.frame = 0;
		img_data.modifier = static_cast<int32_t>(SCSim::ImageModifier::SelectionCircle);

		renderer_->ApplySnapshot(img_data, SCImageParam{ .depth_key = depth_key, .player_color_idx = color_group });
	}
}