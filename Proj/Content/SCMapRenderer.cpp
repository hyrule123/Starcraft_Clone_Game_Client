#include "Content/pch.h"
#include "SCMapRenderer.h"

#include <Engine/Core/Debug.h>

#include <Engine/Resource/Graphics/Material.h>
#include <Engine/Resource/Graphics/Mesh.h>
#include <Engine/Resource/Graphics/RenderPass/ForwardOpaqueRenderPass.h>
#include <Engine/Resource/Graphics/GraphicsShaderSet.h>
#include <Engine/Resource/Graphics/Buffer/ConstantBuffer.h>
#include <Engine/Resource/Graphics/Buffer/TypedBuffer.h>
#include <Engine/Resource/Graphics/Shader/InputLayoutDesc.h>

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

		per_obj_cb_ = ResourceManager::GetInst().Find<ConstantBuffer>("MapInfoCB"_hash);
		ASSERT(per_obj_cb_ != nullptr);
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
	void SCMapRenderer::WritePerObjData(void* ptr)
	{
		ASSERT(map_loader_ != nullptr);

		auto* baker = map_loader_->GetMapBaker();
		ASSERT(baker != nullptr);

		const auto& map_info = baker->GetMapInfo();

		const auto& tileset_data = baker->GetTileSetGPUData(map_info.terrain_type);
		tileset_data.WPE_color_palettes->BindSRV(GraphicsDevice::GetInst().GetContext(), SLOT_T_WPE_INDICES, ShaderStage::kPS);

		MapInfoCB map_info_cb = {};
		map_info_cb.megatile_size = { map_info.megatile_width, map_info.megatile_height };
		memcpy(ptr, &map_info_cb, sizeof(MapInfoCB));
	}
}