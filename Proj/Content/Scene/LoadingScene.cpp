#include "Content/pch.h"
#include "LoadingScene.h"

#include <Engine/Manager/SceneManager.h>

namespace sc
{
	LoadingScene::LoadingScene(std::function<std::unique_ptr<engine::Scene>()> create_scene_func)
		: Super(LoadingScene::kClassConcreteName)
		, create_scene_func_(std::move(create_scene_func))
	{}

	LoadingScene::~LoadingScene()
	{}

	void LoadingScene::Init()
	{
		Super::Init();

		DEBUG_LOG("LoadingScene: Init() called. Starting worker thread to create scene...");
	}

	void LoadingScene::OnActivate()
	{
		Super::OnActivate();

		CHECK_F(create_scene_func_ != nullptr, "LoadingScene: create_scene_func is null");

		worker_ = std::jthread([this]()
			{
				loaded_scene_ = create_scene_func_();
				is_done_.store(true, std::memory_order_release);
			});
	}

	void LoadingScene::OnFrameStart()
	{
		Super::OnFrameStart();

		if (false == is_done_.load(std::memory_order_acquire))
		{
			return;
		}

		worker_.join();

		CHECK_F(loaded_scene_ != nullptr, "LoadingScene: failed to create scene");
		engine::SceneManager::GetInst().ChangeScene(std::move(loaded_scene_));
	}

	void LoadingScene::OnShutdown()
	{
		// 로딩 중 종료: 워커가 끝날 때까지 대기 (완성된 씬은 소멸자에서 메인 스레드가 파괴)
		if (worker_.joinable())
		{
			worker_.join();
		}
		Super::OnShutdown();
	}
}