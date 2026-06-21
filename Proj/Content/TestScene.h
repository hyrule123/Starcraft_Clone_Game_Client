#pragma once

#include <Engine/Game/Scene.h>

namespace engine
{
    class TestScene :
        public Scene
    {
        CLASS_INFO(TestScene, Scene)
    public:
        TestScene();
        virtual ~TestScene() override;

        virtual void Init() override;
    private:
    };
}


