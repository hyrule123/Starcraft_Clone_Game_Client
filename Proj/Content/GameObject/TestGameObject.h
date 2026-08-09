#pragma once
#include <Engine/Game/GameObject.h>

namespace engine
{
    class TestGameObject :
        public GameObject
    {
        ENTITY_INFO(TestGameObject, GameObject)
    public:
        TestGameObject();
        virtual ~TestGameObject() override;

        virtual void Init() override;

    private:
    };
}


