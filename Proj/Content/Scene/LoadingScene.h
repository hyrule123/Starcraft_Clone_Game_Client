#pragma once
#include <Engine/Game/Scene.h>

#include <functional>
#include <thread>
#include <atomic>

namespace engine
{
    class Texture2D;
}

namespace sc
{
    class LoadingScene :
        public engine::Scene
    {
		ENTITY_INFO(LoadingScene, engine::Scene);

    public:
        LoadingScene(std::function<std::unique_ptr<Scene>()> create_scene_func);
		virtual ~LoadingScene() override;

        static std::unique_ptr<LoadingScene> CreateByName(engine::HashedString next_scene_name);

        virtual void Init() override;
    protected:
        virtual void OnActivate() override;
		virtual void OnFrameStart() override;

        virtual void OnShutdown() override;

    private:
        std::function<std::unique_ptr<engine::Scene>()> create_scene_func_ = {};
		std::unique_ptr<engine::Scene> loaded_scene_ = {};
        std::atomic_bool is_done_ = {};
		std::jthread worker_ = {};

        std::shared_ptr<engine::Texture2D> test_tex_ = {};
    };
}


