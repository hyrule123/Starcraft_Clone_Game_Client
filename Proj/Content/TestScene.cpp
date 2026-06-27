#include "Content/pch.h"
#include "TestScene.h"

#include <Engine/Manager/ResourceManager.h>

#include <Engine/Resource/Graphics/Material.h>
#include <Engine/Resource/Graphics/Buffer/Texture2D.h>

#include <Engine/Game/GameObject.h>
#include <Engine/Game/Component/Camera.h>

#include <Engine/Core/Debug.h>

#include <Content/TerranMarine.h>

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

		AddGameObject<TerranMarine>();

		s_ptr<GameObject> camobj = AddGameObject<GameObject>();
		auto cam = camobj->AddComponent<Camera>();
	}
}

