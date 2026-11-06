#include "Content/pch.h"
#include "SceneTransitionTestScene.h"

#include <Engine/Manager/InputManager.h>
#include <Engine/Manager/SceneManager.h>

namespace sc
{
	SceneTransitionTestScene::SceneTransitionTestScene()
		: Super(SceneTransitionTestScene::kClassConcreteName)
	{}

	SceneTransitionTestScene::~SceneTransitionTestScene()
	{}
	void SceneTransitionTestScene::Init()
	{
		Super::Init();
	}

	void SceneTransitionTestScene::OnFrameStart()
	{
		using namespace engine;

		if (InputManager::GetInst().GetKeyDown(KeyCode::kF9))
		{
			SceneManager::GetInst().ChangeScene("MainGameScene");
		}
	}
}