#include "Content/pch.h"
#include "SCImageRenderer.h"

#include <Content/SCData/SCResourceManager.h>
#include <Content/SCData/SCImageLibrary.h>

#include <Content/Scene/MainGameScene.h>

#include <Content/HLSL/SCImage/SCPcxLut.hlsli>
#include <Content/HLSL/SCImage/SCEffectPass.hlsli>

#include <Content/SCData/Constants.h>

#include <Engine/Resource/GPU/Buffer/Texture2DArray.h>
#include <Engine/Resource/GPU/Material.h>

#include <Engine/Game/Component/Transform.h>

#include <type_traits>

namespace sc
{
	// SCSimulator과 HLSL 값 일치 확인
	static_assert(kImageFlagHorizontallyFlipped == SCSim::ImageFlags::HorizontallyFlipped);
	static_assert(kImageFlagYFrozen == SCSim::ImageFlags::YFrozen);

	static_assert(kModifierShadow == static_cast<int32_t>(SCSim::ImageModifier::Shadow));
	static_assert(kModifierCloaking == static_cast<int32_t>(SCSim::ImageModifier::Cloaking));
	static_assert(kModifierCloaked == static_cast<int32_t>(SCSim::ImageModifier::Cloaked));
	static_assert(kModifierDecloaking == static_cast<int32_t>(SCSim::ImageModifier::Decloaking));
	static_assert(kModifierEnemyCloaking == static_cast<int32_t>(SCSim::ImageModifier::EnemyCloaking));
	static_assert(kModifierEnemyCloaked == static_cast<int32_t>(SCSim::ImageModifier::EnemyCloaked));
	static_assert(kModifierEnemyDecloaking == static_cast<int32_t>(SCSim::ImageModifier::EnemyDecloaking));
	static_assert(kModifierDistortion == static_cast<int32_t>(SCSim::ImageModifier::Distortion));
	static_assert(kModifierRemapping == static_cast<int32_t>(SCSim::ImageModifier::Remapping));
	static_assert(kModifierWarpFlash == static_cast<int32_t>(SCSim::ImageModifier::WarpFlash));

	static_assert(kModifierWarpTexture == static_cast<int32_t>(SCSim::ImageModifier::WarpTexture));

	static_assert(std::is_same_v<decltype(SCImageInstance::image_flags), decltype(SCSim::ImageData::flags)>);
	static_assert(std::is_same_v<decltype(SCImageInstance::modifier), decltype(SCSim::ImageData::modifier)>);
	static_assert(std::is_same_v<decltype(SCImageInstance::modifier_data1), decltype(SCSim::ImageData::modifier_data1)>);
	static_assert(std::is_same_v<decltype(SCImageInstance::player_color_idx), decltype(SCImageParam::player_color_idx)>);
	static_assert(sizeof(decltype(SCImageInstance::remapping)) >= sizeof(decltype(ImageDef::remapping)));

	static_assert(std::is_same_v<decltype(SCImageInstance::frozen_frame_top_y), decltype(SCSim::ImageData::frozen_y_value)>);

	namespace
	{
		SCImageVariant GetMaterialVariant(int32_t modifier)
		{
			using SCSim::ImageModifier;

			switch (static_cast<ImageModifier>(modifier))
			{
			case ImageModifier::Normal:
			case ImageModifier::NormalNoHallu:
			case ImageModifier::WarpTexture:
				return SCImageVariant::Opaque;

			case ImageModifier::Cloaking:
			case ImageModifier::Cloaked:
			case ImageModifier::Decloaking:
			case ImageModifier::Remapping:
			case ImageModifier::Shadow:
			case ImageModifier::WarpFlash:
				return SCImageVariant::Effect;

			case ImageModifier::Distortion:
			case ImageModifier::EnemyCloaking:
			case ImageModifier::EnemyDecloaking:
			case ImageModifier::EnemyCloaked:
				return SCImageVariant::Distortion;

			default:
				break;
			}

			// 미구현/미지원 값: Effect PS default(일반 그리기)로 처리
			DEBUG_LOG("처리되지 않은 modifier 값: {}. Effect Material 사용", modifier);
			return SCImageVariant::Effect;
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
	void SCImageRenderer::ApplySnapshot(const SCSim::ImageData& img_data, const SCImageParam& img_param)
	{
		SCResourceManager& resource_manager = SCResourceManager::GetInst();

		const SCImageVariant variant = GetMaterialVariant(img_data.modifier);

		const bool is_image_type_changed = (img_data.image_type != img_data_cache_.image_type);

		if ((img_def_cache_ == nullptr) || is_image_type_changed)
		{
			const ImageDef& img_def = resource_manager.GetSCImageLibrary().GetImageDef(img_data.image_type);
			img_def_cache_ = &img_def;
		}

		// 판정 기준은 렌더러의 실제 상태:
		// Material이 없거나(첫 설정 — image_type 0도 유효하므로 캐시 비교만으로는 부족),
		// 이미지 타입 또는 Material 종류(일반/그림자)가 바뀌었을 때만 교체
		const bool needs_material = (GetMaterial(0) == nullptr)
			|| is_image_type_changed
			|| (variant != GetMaterialVariant(img_data_cache_.modifier));

		
		const bool is_y_frozen = (img_data.flags & SCSim::ImageFlags::YFrozen) != 0;
		const bool was_y_frozen = (img_data_cache_.flags & SCSim::ImageFlags::YFrozen) != 0;
		bool need_update_bounds = is_y_frozen || was_y_frozen;
		
		if (needs_material)
		{
			std::shared_ptr<engine::Material> mtrl_sub0 = resource_manager.GetImageMaterial(img_data.image_type, variant);
			std::shared_ptr<engine::Material> mtrl_sub1 = variant == SCImageVariant::Distortion ? resource_manager.GetImageMaterial(img_data.image_type, SCImageVariant::DistortionSync) : nullptr;

			SetMaterial(0, mtrl_sub0);
			SetMaterial(1, mtrl_sub1);
			
			need_update_bounds = true;
		}

		if (need_update_bounds)
		{
			// 컬링 bounds: 1픽셀 여유
			std::shared_ptr<engine::Texture2DArray> tex = resource_manager.GetSCImageLibrary().GetGrp(img_data.image_type).texture;

			const float hx = static_cast<float>((tex->GetWidth() >> 1) + 1);
			const float hy = static_cast<float>((tex->GetHeight() >> 1) + 1);
			const float canvas_height = static_cast<float>(tex->GetHeight());

			float max_y = hy;
			float min_y = -hy;

			if (is_y_frozen)
			{
				// 원래 그려져야 할 위치(DX 좌표계)
				const float3 world_pos = GetTransform()->GetWorldPosition();

				// -------------- SC 좌표 --------------
				const int2 sc_pos = WorldToScInt(world_pos.x, world_pos.y);	// SC 좌표로 변환

				// SC 기준(y down) 중심에서 프레임 상단까지의 거리. 그림자는 [d, d + canvas_h] 범위
				const float dist_y = static_cast<float>(img_data.frozen_y_value - sc_pos.y);
				// -------------------------------------
			
				//다시 월드 좌표: y 부호 변환 필요, min <-> max, 1픽셀 여유
				min_y = std::min(min_y, -(dist_y + canvas_height + 1.0f));
				max_y = std::max(max_y, -(dist_y - 1.0f));
			}

			SetLocalBoundsOverride(engine::AABB3D{ .min = float3(-hx, min_y, 0.0f), .max = float3(hx, max_y, 0.0f) });
		}
		

		// 매 싱크 덮어쓰기 — WritePerObjectData가 여기서 읽음
		img_data_cache_ = img_data;
		img_param_cache_ = img_param;

		SetCustomSortKey(img_param.depth_key);
	}

	void SCImageRenderer::WriteInstanceData(const engine::Renderer::InstanceWriteArgs& args)
	{
		switch (args.pass)
		{
		case kSCOpaquePassID:
		case kSCEffectPassID:
		{
			// Transform 월드 위치(이미지 중심) → SC 좌표 (VS가 SC 공간에서 배치 후 월드로 변환)
			const float3 world_pos = GetTransform()->GetWorldPosition();

			SCImageInstance data = {};

			data.sc_center_pos = WorldToScInt(world_pos.x, world_pos.y);
			data.frame = img_data_cache_.frame;
			data.depth_key = img_param_cache_.depth_key;
			data.frozen_frame_top_y = img_data_cache_.frozen_y_value;
			data.image_flags = img_data_cache_.flags;
			data.modifier = img_data_cache_.modifier;
			data.modifier_data1 = img_data_cache_.modifier_data1;
			data.player_color_idx = img_param_cache_.player_color_idx;

			ASSERT(img_def_cache_);
			// uint8 → uint
			data.remapping = static_cast<uint>(img_def_cache_->remapping);

			// 의미: Modifier가 Remapping일 경우 1 - 7 범위 사이여야 함
			// 다른 테이블 인덱스를 참조하라는 remap이기 때문
			ASSERT(data.modifier != static_cast<int32_t>(SCSim::ImageModifier::Remapping)
				|| (kPcxLutOfireIdx <= data.remapping && data.remapping < kPcxLutCount));

			args.block.Write(data);

			break;
		}
		default:
			ASSERT_F(false, "처리되지 않은 Pass ID: {}", static_cast<uint32_t>(args.pass));
			break;
		}
	}
}

