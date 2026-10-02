#include "Content/pch.h"
#include "SCImageRenderer.h"

#include <Content/SCData/SCResourceManager.h>
#include <Content/SCData/SCImageLibrary.h>

#include <Content/Scene/MainGameScene.h>

#include <Content/HLSL/SCImage/SCImageCommon.hlsli>

#include <Engine/Resource/GPU/Buffer/Texture2DArray.h>
#include <Engine/Resource/GPU/Material.h>

#include <Engine/Game/Component/Transform.h>


namespace sc
{
	namespace
	{
		constexpr uint8_t kModifierShadow = 10;

		SCImageMaterialVariant GetMaterialVariant(uint8_t modifier)
		{
			return (modifier == kModifierShadow) ? SCImageMaterialVariant::Shadow : SCImageMaterialVariant::Normal;
		}
	}

	SCImageRenderer::SCImageRenderer()
		: Super(SCImageRenderer::kClassConcreteName)
	{
	}

	SCImageRenderer::~SCImageRenderer()
	{}
	void SCImageRenderer::Awake()
	{
		Super::Awake();

		// Mesh와 Material 설정은 ApplySnapshot에서 처리
		ASSERT(IsRenderReady());
	}
	void SCImageRenderer::ApplySnapshot(const SCSim::ImageData& img_data, uint8_t player_color)
	{
		const SCImageMaterialVariant variant = GetMaterialVariant(img_data.modifier);

		// 판정 기준은 렌더러의 실제 상태:
		// Material이 없거나(첫 설정 — image_type 0도 유효하므로 캐시 비교만으로는 부족),
		// 이미지 타입 또는 Material 종류(일반/그림자)가 바뀌었을 때만 교체
		const bool needs_material = (GetMaterial(0) == nullptr)
			|| (img_data.image_type != img_data_cache_.image_type)
			|| (variant != GetMaterialVariant(img_data_cache_.modifier));

		if (needs_material)
		{
			SCResourceManager& resource_manager = SCResourceManager::GetInst();
			std::shared_ptr<engine::Material> new_mtrl = resource_manager.GetImageMaterial(img_data.image_type, variant);

			SetMaterial(0, new_mtrl);

			// 컬링 bounds: 1픽셀 여유
			const auto& tex = resource_manager.GetSCImageLibraryInst().GetTexture(img_data.image_type);

			const float hx = (float)((tex->GetWidth() >> 1) + 1);
			const float hy = (float)((tex->GetHeight() >> 1) + 1);

			SetLocalBoundsOverride(engine::AABB3D{ .min = float3(-hx, -hy, 0.0f), .max = float3(hx, hy, 0.0f) });
		}

		// 매 싱크 덮어쓰기 — WritePerObjectData가 여기서 읽음
		img_data_cache_ = img_data;
		player_color_ = player_color;
	}

	void SCImageRenderer::WritePerObjectData(engine::DataBlock data_block)
	{
		ASSERT(data_block.size == sizeof(SCImageInstance));

		// Transform 월드 위치(이미지 중심) → SC 좌표 (VS가 SC 공간에서 배치 후 월드로 변환)
		const float3 world_pos = GetTransform()->GetWorldPosition();

		const bool flipped = (img_data_cache_.flags & SCSim::ImageFlags::Flipped) != 0;

		SCImageInstance data = {};
		data.sc_center_pos = WorldToScInt(world_pos.x, world_pos.y);
		data.frame = img_data_cache_.frame;
		data.flags = (static_cast<uint32>(flipped) << kSpriteFlippedShift)
			| (static_cast<uint32>(player_color_) << kSpriteColorShift)
			| (static_cast<uint32>(img_data_cache_.modifier) << kSpriteModifierShift);
		data.depth_key = img_data_cache_.depth_key;

		data_block.Write(data);
	}
}

