#pragma once
#include <Engine/Game/Scene.h>

#include <functional>
#include <thread>
#include <atomic>

namespace sc
{
    class Scene;
    class LoadingScene :
        public engine::Scene
    {
		ENTITY_INFO(LoadingScene, engine::Scene);

    public:
        LoadingScene(std::function<std::unique_ptr<Scene>()> createSceneFunc);
		virtual ~LoadingScene() override;

        virtual void Init() override;
        virtual void OnActivate() override;
		virtual void OnFrameStart() override;

        virtual void OnShutdown() override;

    private:
        std::function<std::unique_ptr<Scene>()> create_scene_func_ = {};
		std::unique_ptr<Scene> loaded_scene_ = {};
        std::atomic_bool is_done_ = {};
		std::jthread worker_ = {};
    };
}


