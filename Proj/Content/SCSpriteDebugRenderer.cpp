#include "Content/pch.h"
#include "SCSpriteDebugRenderer.h"

#include <Engine/Manager/ResourceManager.h>

#include <Engine/Resource/GPU/Material.h>
#include <Engine/Resource/GPU/PipelineState.h>

#include <Engine/Game/Component/Transform.h>

#include <Engine/HLSL/Core/Register.hlsli>

#include <Content/HLSL/SCSpriteDebug.hlsli>

namespace engine
{
	SCSpriteDebugRenderer::SCSpriteDebugRenderer()
		: Super(SCSpriteDebugRenderer::kClassConcreteName)
	{}
	SCSpriteDebugRenderer::~SCSpriteDebugRenderer()
	{}
	void SCSpriteDebugRenderer::Init()
	{
		Super::Init();

		bool result = SetMesh("Mesh_Standard2D_Rect"_hash);
		CHECK(result);

		s_ptr<Material> material = ResourceManager::GetInst().Find<Material>("SCUnitDebugMaterial"_hash);
		if (material == nullptr)
		{
			material = EntityManager::CreateEntity<Material>();
			ResourceManager::GetInst().AddResource("SCUnitDebugMaterial"_hash, material);

			s_ptr<PipelineState> pipeline_state = EntityManager::GetInst().CreateEntity<PipelineState>();

			pipeline_state->SetVertexShader("Shader/SCSpriteDebug_VS.cso"_hash);
			pipeline_state->SetPixelShader("Shader/SCSpriteDebug_PS.cso"_hash);
			pipeline_state->CreateInputLayout("InputLayoutDesc_Standard2D"_hash);

			pipeline_state->SetDepthStencilState("DSS_Default"_hash);
			pipeline_state->SetRasterizerState("RSS_Solid_Back"_hash);

			pipeline_state->SetPerInstanceDataStride<SpriteInstanceData>();
			pipeline_state->SetInstancingSupport(true);

			material->SetPipelineState(RenderPassOrder::ForwardOpaque, pipeline_state);
		}
		material = material->Clone();
		result = SetMaterial(0, material);
		CHECK(result);

		material->SetTexture(REG_T_PER_MATERIAL_0, "Texture2D/Magenta.png"_hash);
	}

	void SCSpriteDebugRenderer::WritePerObjectData(DataBlock data_block)
	{
		Transform* transform = GetTransform();
		
		SpriteInstanceData inst_data = {};
		inst_data.world_mat = transform->GetWorldMatrix();
		data_block.Write(inst_data);

		int a = 3;
	}
}