#include "Content/pch.h"
#include "TestScene.h"

#include <Engine/Manager/GraphicsDevice.h>
#include <Engine/Manager/ResourceManager.h>
#include <Engine/Resource/Mesh/Mesh.h>
#include <Engine/Resource/Graphics/InputLayout.h>
#include <Engine/Resource/Graphics/VertexShader.h>
#include <Engine/Resource/Graphics/PixelShader.h>


namespace engine
{
	TestScene::TestScene()
		: Scene("TestScene")
	{
	}

	TestScene::~TestScene()
	{
	}

	void TestScene::Init()
	{
		Scene::Init();

		auto& resmgr = ResourceManager::GetInst();

		il_ = resmgr.Find<InputLayout>("Debug_IL");
		vs_ = resmgr.Find<VertexShader>("Shader/Debug_VS.cso");
		ps_ = resmgr.Find<PixelShader>("Shader/Debug_PS.cso");
		msh_ = resmgr.Find<Mesh>("DebugMesh");

		if (!(il_ && vs_ && ps_ && msh_))
		{
			ERROR_MESSAGE("뭔가 로딩 안됨");
		}
	}
	void TestScene::Render()
	{
		Scene::Render();

		auto context = GraphicsDevice::GetInst().GetContext();

		il_->Bind(context);
		vs_->Bind(context);
		ps_->Bind(context);

		msh_->Render();
	}
}

