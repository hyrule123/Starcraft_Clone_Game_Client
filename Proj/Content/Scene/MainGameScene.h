#pragma once

#include <Engine/Game/Scene.h>

namespace engine
{
    class MainGameScene :
        public Scene
    {
        CLASS_INFO(MainGameScene, Scene)
    public:
        MainGameScene();
        virtual ~MainGameScene() override;

        virtual void Init() override;
    private:
		void LoadResources();
    };
}


