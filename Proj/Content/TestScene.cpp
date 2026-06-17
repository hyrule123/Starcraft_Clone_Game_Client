#include "Content/pch.h"
#include "TestScene.h"

#include <Engine/Manager/GraphicsDevice.h>
#include <Engine/Manager/ResourceManager.h>
#include <Engine/Resource/Mesh/Mesh.h>
#include <Engine/Resource/GraphicsPipeline/InputLayout.h>
#include <Engine/Resource/GraphicsPipeline/VertexShader.h>
#include <Engine/Resource/GraphicsPipeline/PixelShader.h>

#include <Engine/Game/GameObject.h>
#include <Engine/Game/Component/Transform.h>

#include <Engine/Core/Debug.h>

#include <Content/TestGameObject.h>

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

		s_ptr<TestGameObject> testobj = std::make_shared<TestGameObject>();

		AddGameObject(testobj);

		int a = 3;
	}
	void TestScene::Update()
	{
		Super::Update();
	}
	void TestScene::Render()
	{
		Super::Render();


	}
}

