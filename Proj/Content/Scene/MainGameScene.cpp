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
		//one->GetTransform()->SetLocalPosition({ -200.0f, 0, 10.0f });
		//auto two = AddGameObject<TerranMarine>();
		//two->GetTransform()->SetLocalPosition({ 100.0f, 0, 10.0f });
		//
		//auto three = AddGameObject<TerranMedic>();
		//three->GetTransform()->SetLocalPosition({ 400.0f, 0, 10.0f });

		s_ptr<GameObject> camobj = AddGameObject<GameObject>();
		auto cam = camobj->AddComponent<Camera>();
	}
	void MainGameScene::LoadResources()
	{
		auto& res_mgr = ResourceManager::GetInst();
		auto& resmgr = ResourceManager::GetInst();

		auto device = GraphicsDevice::GetInst().GetDevice();
	}
}

