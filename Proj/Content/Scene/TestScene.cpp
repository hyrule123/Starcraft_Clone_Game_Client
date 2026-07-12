#include "Content/pch.h"
#include "TestScene.h"

#include <Engine/Manager/ResourceManager.h>

#include <Engine/Resource/Graphics/Material.h>
#include <Engine/Resource/Graphics/Buffer/Texture2D.h>

#include <Engine/Game/GameObject.h>
#include <Engine/Game/Component/Camera.h>
#include <Engine/Game/Component/Transform.h>

#include <Engine/Core/Debug.h>

#include <Content/GameObject/TerranMarine.h>
#include <Content/GameObject/TerranMedic.h>

namespace engine
{
	TestScene::TestScene()
		: Scene(STRINGIFY(TestScene))
	{
	}

	TestScene::~TestScene()
	{
	}

	void TestScene::Init()
	{
		Super::Init();

		auto one = AddGameObject<TerranMarine>();
		one->GetTransform()->SetLocalPosition({ -200.0f, 0, 10.0f });
		auto two = AddGameObject<TerranMarine>();
		two->GetTransform()->SetLocalPosition({ 100.0f, 0, 10.0f });
		
		auto three = AddGameObject<TerranMedic>();
		three->GetTransform()->SetLocalPosition({ 400.0f, 0, 10.0f });

		s_ptr<GameObject> camobj = AddGameObject<GameObject>();
		auto cam = camobj->AddComponent<Camera>();
	}
}

