#include "Content/pch.h"
#include "SCMapRenderer.h"

#include <Engine/Core/Debug.h>

#include <Engine/Resource/GPU/Material.h>
#include <Engine/Resource/GPU/Mesh.h>
#include <Engine/Render/RenderPass/ForwardOpaqueRenderPass.h>
#include <Engine/Resource/GPU/PipelineState.h>
#include <Engine/Resource/GPU/Buffer/ConstantBuffer.h>
#include <Engine/Resource/GPU/Buffer/TypedBuffer.h>
#include <Engine/Resource/GPU/Shader/InputLayoutDesc.h>

#include <Engine/Manager/RenderManager.h>
#include <Engine/Manager/ResourceManager.h>
#include <Engine/Manager/GraphicsDevice.h>

#include <Engine/Game/Component/Transform.h>
#include <Engine/Game/Component/Camera.h>

#include <Content/SCData/SCMapLoader.h>
#include <Content/SCData/SCMapBakeComputePass.h>

#include <Content/HLSL/SCMapRender.hlsli>

namespace sc
{
	constexpr engine::RenderPassOrder kMapRenderPass = engine::RenderPassOrder::ForwardOpaque;


	SCMapRenderer::SCMapRenderer()
		: Super(SCMapRenderer::kClassConcreteName)
	{}
	SCMapRenderer::~SCMapRenderer()
	{}
	void SCMapRenderer::Init()
	{
		Super::Init();

		using namespace engine;
		using namespace engine::literals;

		bool result = SetMesh("Mesh_Standard2D_Rect");
		ASSERT(result);

		constexpr HashedStringView material_name = "Material_SCMap";
		std::shared_ptr<Material> map_mtrl = ResourceManager::GetInst().Find<Material>(material_name);

		if (map_mtrl == nullptr)
		{
			map_mtrl = EntityManager::CreateEntity<Material>();

			std::shared_ptr<PipelineState> pipeline_state = EntityManager::GetInst().CreateEntity<PipelineState>();
			pipeline_state->SetInstancingSupport(false);
			pipeline_state->SetPerInstanceDataStride(sizeof(MapInfoCB));
			pipeline_state->SetVertexShader("Shader/SCMapRender_VS.cso");
			pipeline_state->SetPixelShader("Shader/SCMapRender_PS.cso");
			pipeline_state->SetDepthStencilState("DSS_Default");
			pipeline_state->SetRasterizerState("RSS_Solid_Back");
			pipeline_state->CreateInputLayout("InputLayoutDesc_Standard2D");
			map_mtrl->SetPipelineState(kMapRenderPass, pipeline_state);

			ResourceManager::GetInst().AddResource(material_name, map_mtrl);
		}
		ASSERT(map_mtrl);
		SetMaterial(0, map_mtrl);

		PipelineState* pipeline_state = map_mtrl->GetPipelineState(kMapRenderPass);
		ASSERT(pipeline_state);

		SetEnable(false);
	}
	void SCMapRenderer::LateUpdate()
	{
		using namespace engine;

		Super::LateUpdate();

		if (sc_map_ == nullptr)
		{
			DEBUG_LOG("SCMap is not set.");
			return;
		}

		if (!IsRenderReady())
		{
			DEBUG_LOG("SCMapRenderer is not ready to render. Material or Mesh is missing.");
			return;
		}

		if (needs_scale_matching_)
		{
			// SC 좌표계: 왼쪽 위가 (0,0), 오른쪽 아래가 (width, height) -> +x right, +y down
			// 원점을 기준으로 2사분면을 사용한다.
			Transform* tr = GetTransform();
			const float width = (float)sc_map_->megatile_width * MEGATILE_PIXEL_SIZE;
			const float height = (float)sc_map_->megatile_height * MEGATILE_PIXEL_SIZE;
			const float2 center = float2(width * 0.5f, - (height * 0.5f));

			Camera* cam = RenderManager::GetInst().GetMainCamera().get();
			CHECK(cam);

			const auto& desc = cam->GetProjectionMatrixDesc();
			float3 cam_pos = cam->GetComponent<Transform>()->GetLocalPosition();

			// 카메라를 맵의 중앙으로 이동
			cam_pos.x = center.x;
			cam_pos.y = center.y;
			cam->GetComponent<Transform>()->SetLocalPosition(cam_pos);

			// 맵을 카메라로부터 제일 멀리 떨어뜨려서 배치
			const float farthest_z = desc.far_z + cam_pos.z - 0.01f;
			tr->SetLocalPosition(float3(width * 0.5f, - (height * 0.5f), farthest_z));
			tr->SetLocalScale(float3(width, height, 1.0f));
			
			needs_scale_matching_ = false;
		}
	}
	void SCMapRenderer::OnDestroy()
	{
		using namespace engine;

		Super::OnDestroy();
		auto& per_pass_table = RenderManager::GetInst().GetPerFrameBindingTable();

		per_pass_table.RemoveShaderResource(REG_T_SC_WPE_PALETTE);
		per_pass_table.RemoveShaderResource(REG_T_SC_PCX_TILESET_LUT);	
	}
	void SCMapRenderer::WritePerObjectData(engine::DataBlock data_block)
	{
		using namespace engine;

		MapInfoCB map_info_cb = {};
		map_info_cb.megatile_size = { (int32)sc_map_->megatile_width, (int32)sc_map_->megatile_height };
		map_info_cb.world_mat = GetTransform()->GetWorldMatrix();

		data_block.Write(map_info_cb);

		//if (sc_map_ == nullptr)
		//{
		//	ASSERT(false);
		//	return;
		//}
		//if(false == data_block.IsValid())
		//{
		//	ASSERT(false);
		//	return;
		//}

		//auto* context = GraphicsDevice::GetInst().GetContext();

		////TODO: 임시방편, Shader Set 단위로 묶는 함수가 필요할 듯
		//sc_map_->wpe_color_palettes->BindSRV(context, REG_T_SC_WPE_PALETTE, ShaderStage::kPS);

		//MapInfoCB map_info_cb = {};
		//map_info_cb.megatile_size = { sc_map_->megatile_width, sc_map_->megatile_height };

		//data_block.Write(map_info_cb);
	}
	void SCMapRenderer::SetSCMap(std::unique_ptr<SCMap> sc_map)
	{
		using namespace engine;

		if (sc_map == nullptr)
		{
			SetEnable(false);
			return;
		}

		sc_map_ = std::move(sc_map);

		Material* mtrl = GetMaterial(0);
		ASSERT(mtrl);
		
		mtrl->SetTexture(ShaderStage::Pixel, REG_T_MAP_TEXTURE, sc_map_->texture);

		auto& per_pass_table = RenderManager::GetInst().GetPerFrameBindingTable();
		per_pass_table.RemoveShaderResource(REG_T_SC_WPE_PALETTE);
		per_pass_table.AddShaderResource(ShaderStage::Vertex | ShaderStage::Pixel, REG_T_SC_WPE_PALETTE, sc_map_->wpe_color_palettes);

		per_pass_table.RemoveShaderResource(REG_T_SC_PCX_TILESET_LUT);
		per_pass_table.AddShaderResource(ShaderStage::Vertex | ShaderStage::Pixel, REG_T_SC_PCX_TILESET_LUT, sc_map_->pcx_tileset_LUT);

		needs_scale_matching_ = true;
		SetEnable(true);
	}
}