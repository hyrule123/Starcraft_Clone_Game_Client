#include "Content/pch.h"
#include "MainGameScene.h"

#include <Engine/Manager/GraphicsDevice.h>
#include <Engine/Manager/ResourceManager.h>

#include <Engine/Resource/Graphics/Material.h>
#include <Engine/Resource/Graphics/Mesh.h>
#include <Engine/Resource/Graphics/GraphicsShaderSet.h>

#include <Engine/Game/GameObject.h>
#include <Engine/Game/Component/Camera.h>
#include <Engine/Game/Component/Transform.h>

#include <Engine/Core/Debug.h>

#include <Content/GameObject/TerranMarine.h>
#include <Content/GameObject/TerranMedic.h>

#include <Content/Shader/SCSprite.hlsli>

namespace engine
{
	MainGameScene::MainGameScene()
		: Scene(STRINGIFY(MainGameScene))
	{
	}

	MainGameScene::~MainGameScene()
	{
	}

	void MainGameScene::Init()
	{
		Super::Init();

		LoadResources();

		auto one = AddGameObject<TerranMarine>();
		one->GetTransform()->SetLocalPosition({ -200.0f, 0, 10.0f });
		auto two = AddGameObject<TerranMarine>();
		two->GetTransform()->SetLocalPosition({ 100.0f, 0, 10.0f });
		
		auto three = AddGameObject<TerranMedic>();
		three->GetTransform()->SetLocalPosition({ 400.0f, 0, 10.0f });

		s_ptr<GameObject> camobj = AddGameObject<GameObject>();
		auto cam = camobj->AddComponent<Camera>();
	}
	void MainGameScene::LoadResources()
	{
		auto& res_mgr = ResourceManager::GetInst();
		auto& resmgr = ResourceManager::GetInst();

		auto device = GraphicsDevice::GetInst().GetDevice();

		s_ptr<GraphicsShaderSet> shader_set = std::make_shared<GraphicsShaderSet>();
		shader_set->SetVertexShader("Shader/SCSprite_VS.cso"_hash);
		shader_set->CreateInputLayout("InputLayoutDesc_Standard2D"_hash);
		shader_set->SetPixelShader("Shader/SCSprite_PS.cso"_hash);
		shader_set->SetInstancingSupport(true);
		shader_set->SetPerInstanceDataStride(sizeof(SCSpriteInstanceData));
		shader_set->SetDepthStencilState("DSS_Default"_hash);
		shader_set->SetRasterizerState("RSS_Solid_Back"_hash);

		resmgr.AddResource("GraphicsShaderSet_SCSprite"_hash, shader_set);

		s_ptr<Mesh> msh = resmgr.Find<Mesh>("Mesh_Standard2D_Rect"_hash);
		ASSERT(msh);

		//MATERIAL
		s_ptr<Material> material = std::make_shared<Material>();
		material->SetShaderSet(shader_set, RenderPassOrder::kForwardOpaque);
		resmgr.AddResource("Material_SCSprite"_hash, material);
	}
}

