#include "Content/pch.h"
#include "SCSpriteRenderer.h"

#include <Engine/Manager/ResourceManager.h>
#include <Engine/Manager/RenderManager.h>

#include <Engine/Resource/Graphics/Material.h>
#include <Engine/Resource/Graphics/Mesh.h>
#include <Engine/Resource/Graphics/Shader/VertexShader.h>

#include <Engine/Game/Component/Transform.h>

#include <Engine/Core/Debug.h>

namespace engine
{
	SCSpriteRenderer::SCSpriteRenderer()
		: Super(SCSpriteRenderer::kClassConcreteName)
	{}
	SCSpriteRenderer::~SCSpriteRenderer()
	{}

	void SCSpriteRenderer::Init()
	{
		Super::Init();
		bool result = SetMesh("Mesh_Standard2D_Rect"_hash);
		result = (result && SetMaterial("Material_Sprite"_hash));
		ASSERT(result);
	}
	void SCSpriteRenderer::Awake()
	{
		Super::Awake();
		ASSERT(sizeof(per_obj_data_) == GetInstanceDataStride(render_pass_mode_));
	}
	void SCSpriteRenderer::LateUpdate()
	{
		Super::LateUpdate();

		if (!IsRenderReady())
		{
			DEBUG_LOG("SCSpriteRenderer is not ready to render. Material or Mesh is missing.");
			return;
		}

		if (render_pass_mode_ == RenderPassOrder::kForwardOpaque
			&&
			GetMaterial()->IsReady(render_pass_mode_))
		{
			auto* opaque_pass = RenderManager::GetInst().GetOpaquePass();

			ForwardOpaqueRenderPass::RenderItem item;
			item.key.material_id = GetMaterial()->GetInstanceID();
			item.key.mesh_id = GetMesh()->GetInstanceID();
			item.renderer = this;

			opaque_pass->SubmitRenderItem(item);
		}
	}
	void SCSpriteRenderer::WritePerObjData(void* ptr)
	{
		per_obj_data_.world_mat = GetTransform()->GetWorldMatrix();
		memcpy(ptr, &per_obj_data_, sizeof(per_obj_data_));
	}
}