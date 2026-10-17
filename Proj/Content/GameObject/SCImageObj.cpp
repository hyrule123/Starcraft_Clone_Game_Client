#include "Content/pch.h"
#include "SCImageObj.h"

#include <Engine/Game/Component/Transform.h>

#include <Content/Component/SCImageRenderer.h>

#include <Content/SCData/SCResourceManager.h>

#include <limits>

namespace sc
{
	SCImageObj::SCImageObj()
		: Super(SCImageObj::kClassConcreteName)
	{}

	SCImageObj::~SCImageObj()
	{}
	void SCImageObj::Init()
	{
		Super::Init();

		// Renderer 부착
		renderer_ = AddComponent<SCImageRenderer>();

		// Mesh는 불변
		renderer_->SetMesh(SCResourceManager::GetInst().GetImageMesh());
	}
	void SCImageObj::ApplySnapshot(const SCSim::ImageData& img_data, const SCImageParam& img_param)
	{
		// 부모 = SCSpriteObj -> Sprite로부터 오프셋 위치 변경
		// Z값: 부모인 SCSpriteObj에 로컬로 영향을 받음(kSpriteDefaultZ = 0.5f)
		const float3 local_offset_new = { static_cast<float>(img_data.offset_x), static_cast<float>(-img_data.offset_y), 0.0f };
		engine::Transform* transform = GetTransform();
		const float3 local_offset_old = transform->GetLocalPosition();

		if (local_offset_new != local_offset_old)
		{
			transform->SetLocalPosition(local_offset_new);
		}
		
		// image_data의 변경 사항을 기준으로 데이터 변경
		renderer_->ApplySnapshot(img_data, img_param);
	}
}