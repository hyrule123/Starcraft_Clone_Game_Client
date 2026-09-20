#pragma once

#include <Engine/Game/Scene.h>

struct SCSimContext;

namespace engine
{
    class MainGameScene :
        public Scene
    {
        ENTITY_INFO(MainGameScene, Scene)
    public:
        MainGameScene();
        virtual ~MainGameScene() override;

        virtual void Init() override;
        virtual void FrameStart() override;
    private:
        void PrepareMapLoader();
		void LoadResources();

    private:
        SCSimContext* sc_sim_ctx_ = nullptr;
		float acc_delta_time_ = 0.0f; // for SCSimContext update
    };
}


