#pragma once
#include <Engine/Game/GameObject.h>

namespace sc
{
    class TestGameObject :
        public engine::GameObject
    {
        ENTITY_INFO(TestGameObject, engine::GameObject)
    public:
        TestGameObject();
        virtual ~TestGameObject() override;

        virtual void Init() override;

    private:
    };
}


