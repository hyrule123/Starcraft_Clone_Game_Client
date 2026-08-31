#include "Content/pch.h"
#include "SCMapRenderer.h"

#include <Engine/Core/Debug.h>

#include <Engine/Resource/GPU/Material.h>
#include <Engine/Resource/GPU/Mesh.h>
#include <Engine/Resource/GPU/RenderPass/ForwardOpaqueRenderPass.h>
#include <Engine/Resource/GPU/GraphicsShaderSet.h>
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

		s_ptr<GraphicsShaderSet> shader_set = EntityManager::GetInst().CreateEntity<GraphicsShaderSet>();

		shader_set->SetInstancingSupport(false);
		shader_set->SetPerInstanceDataStride(sizeof(MapInfoCB));
		shader_set->SetVertexShader("Shader/SCMapRender_VS.cso"_hash);
		shader_set->SetPixelShader("Shader/SCMapRender_PS.cso"_hash);
		shader_set->SetDepthStencilState("DSS_Default"_hash);
		shader_set->SetRasterizerState("RSS_Solid_Back"_hash);

		shader_set->CreateInputLayout("InputLayoutDesc_Standard2D"_hash);

		s_ptr<Material> map_mtrl = ResourceManager::GetInst().Find<Material>("Material_Sprite"_hash);
		map_mtrl = map_mtrl->Clone();
		map_mtrl->SetShaderSet(shader_set, RenderPassOrder::kForwardOpaque);
		SetMaterial(map_mtrl);

		//per_obj_cb_ = ResourceManager::GetInst().Find<ConstantBuffer>("MapInfoCB"_hash);
		//ASSERT(per_obj_cb_ != nullptr);
	}
	void SCMapRenderer::LateUpdate()
	{
		Super::LateUpdate();

		// TODO: SpriteRenderer와 동일한 로직을 사용하고 있음. 통합할 것
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
		}
	}
	void SCMapRenderer::WritePerObjData(DataBlock data_block)
	{
		if (sc_map_ == nullptr)
		{
			ASSERT(false);
			return;
		}
		if(false == data_block.IsValid())
		{
			ASSERT(false);
			return;
		}

		auto* context = GraphicsDevice::GetInst().GetContext();

		//TODO: 임시방편, Shader Set 단위로 묶는 함수가 필요할 듯
		sc_map_->wpe_color_palettes->BindSRV(context, SLOT_T_WPE_INDICES, ShaderStage::kPS);

		MapInfoCB map_info_cb = {};
		map_info_cb.megatile_size = { sc_map_->megatile_width, sc_map_->megatile_height };

		data_block.Write(map_info_cb);
	}
	void SCMapRenderer::SetSCMap(u_ptr<SCMap> sc_map)
	{
		sc_map_ = std::move(sc_map);
		GetMaterial()->SetTexture(sc_map_->texture, SLOT_T_MAP_TEXTURE);
	}
}