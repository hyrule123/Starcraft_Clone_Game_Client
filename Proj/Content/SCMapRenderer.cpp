#include "Content/pch.h"
#include "SCMapRenderer.h"

#include <Engine/Core/Debug.h>

#include <Engine/Resource/GPU/Material.h>
#include <Engine/Resource/GPU/Mesh.h>
#include <Engine/Resource/GPU/RenderPass/ForwardOpaqueRenderPass.h>
#include <Engine/Resource/GPU/PipelineState.h>
#include <Engine/Resource/GPU/Buffer/ConstantBuffer.h>
#include <Engine/Resource/GPU/Buffer/TypedBuffer.h>
#include <Engine/Resource/GPU/Shader/InputLayoutDesc.h>

#include <Engine/Manager/RenderManager.h>
#include <Engine/Manager/ResourceManager.h>
#include <Engine/Manager/GraphicsDevice.h>

#include <Content/SCData/SCMapLoader.h>
#include <Content/SCData/SCMapBakeComputePass.h>

#include <Content/HLSL/SCMapRender.hlsli>

namespace engine
{
	constexpr RenderPassOrder kMapRenderPass = RenderPassOrder::kForwardOpaque;


	SCMapRenderer::SCMapRenderer()
		: Super(SCMapRenderer::kClassConcreteName)
	{}
	SCMapRenderer::~SCMapRenderer()
	{}
	void SCMapRenderer::Init()
	{
		Super::Init();

		bool result = SetMesh("Mesh_Standard2D_Rect"_hash);
		ASSERT(result);

		constexpr HashedStringView material_name = "Material_SCMap"_hash;
		s_ptr<Material> map_mtrl = ResourceManager::GetInst().Find<Material>(material_name);
		s_ptr<PipelineState> pipeline_state = nullptr;

		if (map_mtrl == nullptr)
		{
			map_mtrl = EntityManager::CreateEntity<Material>();

			s_ptr<PipelineState> pipeline_state = EntityManager::GetInst().CreateEntity<PipelineState>();
			pipeline_state->SetInstancingSupport(false);
			pipeline_state->SetPerInstanceDataStride(sizeof(MapInfoCB));
			pipeline_state->SetVertexShader("Shader/SCMapRender_VS.cso"_hash);
			pipeline_state->SetPixelShader("Shader/SCMapRender_PS.cso"_hash);
			pipeline_state->SetDepthStencilState("DSS_Default"_hash);
			pipeline_state->SetRasterizerState("RSS_Solid_Back"_hash);
			pipeline_state->CreateInputLayout("InputLayoutDesc_Standard2D"_hash);
			map_mtrl->SetPipelineState(kMapRenderPass, pipeline_state);

			ResourceManager::GetInst().AddResource(material_name, map_mtrl);
		}
		ASSERT(map_mtrl);
		SetMaterial(map_mtrl);

		pipeline_state = map_mtrl->GetPipelineState(kMapRenderPass);
		ASSERT(pipeline_state);

		constexpr HashedStringView map_info_cb_name = "MapInfoCB"_hash;
		s_ptr<ConstantBuffer> map_info_cb = ResourceManager::GetInst().Find<ConstantBuffer>(map_info_cb_name);
		if (map_info_cb == nullptr)
		{
			s_ptr<ConstantBuffer> map_info_cb = EntityManager::CreateEntity<ConstantBuffer>();
			map_info_cb->Create<MapInfoCB>();
			pipeline_state->AddConstantBufferBinding(ShaderStage::Flags::Vertex, REG_B_MAP_INFO, map_info_cb);
			
			map_info_cb_ = map_info_cb;

			ResourceManager::GetInst().AddResource(map_info_cb_name, map_info_cb);
		}
	}
	void SCMapRenderer::LateUpdate()
	{
		Super::LateUpdate();

		if (sc_map_ == nullptr)
		{
			DEBUG_MESSAGE("SCMap is not set. Please call SetSCMap() before rendering.");
			return;
		}

		if (!IsRenderReady())
		{
			DEBUG_LOG("SCMapRenderer is not ready to render. Material or Mesh is missing.");
			return;
		}
		if (GetMaterial()->IsReady(RenderPassOrder::kForwardOpaque))
		{
			auto* opaque_pass = RenderManager::GetInst().GetOpaquePass();
			ForwardOpaqueRenderPass::RenderItem item;
			item.key.material_id = GetMaterial()->GetInstanceID();
			item.key.mesh_id = GetMesh()->GetInstanceID();
			item.renderer = this;
			opaque_pass->SubmitRenderItem(item);

			MapInfoCB map_info_cb = {};
			map_info_cb.megatile_size = { sc_map_->megatile_width, sc_map_->megatile_height };
			map_info_cb_->Upload(GraphicsDevice::GetInst().GetContext(), map_info_cb);
		}
	}
	void SCMapRenderer::WritePerInstanceData(DataBlock data_block)
	{
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
		//sc_map_->wpe_color_palettes->BindSRV(context, REG_T_WPE_INDICES, ShaderStage::kPS);

		//MapInfoCB map_info_cb = {};
		//map_info_cb.megatile_size = { sc_map_->megatile_width, sc_map_->megatile_height };

		//data_block.Write(map_info_cb);
	}
	void SCMapRenderer::SetSCMap(u_ptr<SCMap> sc_map)
	{
		s_ptr<Texture2D> map_texture = nullptr;
		s_ptr<ShaderResource> wpe_color_palettes = nullptr;
		if (sc_map)
		{
			map_texture = sc_map->texture;
			wpe_color_palettes = sc_map->wpe_color_palettes;
		}
		else
		{
			map_texture = nullptr;
			wpe_color_palettes = nullptr;
		}

		s_ptr<Material> mtrl = GetMaterial();
		ASSERT(mtrl);
		
		mtrl->SetTexture(REG_T_MAP_TEXTURE, map_texture);
		s_ptr<PipelineState> pipeline_state = mtrl->GetPipelineState(kMapRenderPass);
		ASSERT(pipeline_state);
		pipeline_state->AddShaderResourceBinding(ShaderStage::Flags::Pixel, REG_T_WPE_INDICES, wpe_color_palettes);

		sc_map_ = std::move(sc_map);
	}
}