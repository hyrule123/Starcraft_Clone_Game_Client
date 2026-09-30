#include "Content/pch.h"
#include "SCSpriteDebugRenderer.h"

#include <Engine/Manager/ResourceManager.h>

#include <Engine/Resource/GPU/Material.h>
#include <Engine/Resource/GPU/PipelineState.h>
#include <Engine/Resource/GPU/Buffer/Texture2DArray.h>

#include <Engine/Game/Component/Transform.h>

#include <Engine/HLSL/Core/Register.hlsli>

#include <Content/SCData/SCImageLibrary.h>
#include <Content/SCData/SCSpriteCommon.h>
#include <Content/SCData/MPQ/MPQArchive.h>

#include <Content/GameObject/SCUnitDebug.h>

#include <Content/Scene/MainGameScene.h>

#include <Content/HLSL/SCSpriteDebug.hlsli>

#include <SCSimulator/Include/SCSimulatorAPI.h>

namespace sc
{
	using namespace engine;

	SCSpriteDebugRenderer::SCSpriteDebugRenderer()
		: Super(SCSpriteDebugRenderer::kClassConcreteName)
	{}
	SCSpriteDebugRenderer::~SCSpriteDebugRenderer()
	{}
	void SCSpriteDebugRenderer::Init()
	{
		
		Super::Init();

		bool result = SetMesh("Mesh_Standard2D_Rect");
		CHECK(result);

		std::shared_ptr<Material> material = ResourceManager::GetInst().Find<Material>("SCUnitDebugMaterial");
		if (material == nullptr)
		{
			material = EntityManager::CreateEntity<Material>();
			ResourceManager::GetInst().AddResource("SCUnitDebugMaterial", material);

			std::shared_ptr<PipelineState> pipeline_state = EntityManager::GetInst().CreateEntity<PipelineState>();

			pipeline_state->SetVertexShader("Shader/SCSpriteDebug_VS.cso");
			pipeline_state->SetPixelShader("Shader/SCSpriteDebug_PS.cso");
			pipeline_state->CreateInputLayout("InputLayoutDesc_Standard2D");

			pipeline_state->SetDepthStencilState("DSS_Default");
			pipeline_state->SetRasterizerState("RSS_Solid_Back");

			pipeline_state->SetPerInstanceDataStride<SCSpriteInstance>();
			pipeline_state->SetInstancingSupport(true);

			material->SetPipelineState(RenderPassOrder::ForwardOpaque, pipeline_state);
		}
		material = material->Clone();
		result = SetMaterial(0, material);
		CHECK(result);

		//material->SetTexture(ShaderStage::Vertex | ShaderStage::Pixel, REG_T_SC_GRP_SPRITE, "Texture2D/Magenta.png");



	}

	void SCSpriteDebugRenderer::Awake()
	{
		Super::Awake();

		Transform* tr = GetComponent<Transform>();
		tr->SetLocalPosition(float3(2048, -2048, 0));
	}

	void SCSpriteDebugRenderer::LateUpdate()
	{
		Super::LateUpdate();

		SCUnitDebug* unit_debug = dynamic_cast<SCUnitDebug*>(GetOwnerGameObject());
		CHECK(unit_debug);
		uint32 unit_id = unit_debug->GetUnitId();

		MainGameScene* scene = dynamic_cast<MainGameScene*>(unit_debug->GetOwnerScene());
		CHECK(scene);

		SCSim::ImageInfo image_info = {};

		bool result = SCSim_GetUnitMainImage(scene->GetSCSimContext(), unit_id, &image_info);
		CHECK(result);

		// 이미지 타입이 바뀔 때만 텍스처 교체 (변태·사망 애니메이션 등)
		if (image_info.image_type != bound_image_type_)
		{
			SCImageLibrary* image_library = scene->GetSCImageLibrary();
			CHECK(image_library);
			const sc::ImageDef& image_def = image_library->GetImageDef(image_info.image_type);

			std::shared_ptr<Texture2D> texture = image_library->GetTexture(image_info.image_type);
			ASSERT(texture != nullptr);   // 유닛 본체 이미지는 항상 GRP가 있어야 함
			GetMaterial(0)->RemoveTexture(REG_T_SC_GRP_SPRITE);
			GetMaterial(0)->SetTexture(ShaderStage::Vertex | ShaderStage::Pixel, REG_T_SC_GRP_SPRITE, texture);   // PerMaterial 슬롯 t30
			bound_image_type_ = image_info.image_type;
		}
	}

	void SCSpriteDebugRenderer::WritePerObjectData(DataBlock data_block)
	{
		SCUnitDebug* unit_debug = dynamic_cast<SCUnitDebug*>(GetOwnerGameObject());
		CHECK(unit_debug);
		uint32 unit_id = unit_debug->GetUnitId();

		MainGameScene* scene = dynamic_cast<MainGameScene*>(unit_debug->GetOwnerScene());
		CHECK(scene);

		SCSim::ImageInfo image_info = {};

		bool result = SCSim_GetUnitMainImage(scene->GetSCSimContext(), unit_id, &image_info);
		CHECK(result);
		
		SCSpriteInstance inst_data = {};

		inst_data.position = float2((float)image_info.x, (float)image_info.y);
		inst_data.frame = image_info.frame;

		sc::SCImageLibrary* image_library = scene->GetSCImageLibrary();
		CHECK(image_library);
		const sc::ImageDef& image_def = image_library->GetImageDef(image_info.image_type);

		inst_data.flags = PackSpriteFlags(
			image_info.flipped != 0,
			image_info.player_color,
			0,                       // modifier: ImageInfo에 추가 전까지 0 (일반 그리기)
			0,                       // modifier_data1: 동일
			image_def.remapping);    // LUT 테이블 번호

		data_block.Write(inst_data);
	}
}