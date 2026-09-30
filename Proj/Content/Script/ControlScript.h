#pragma once
#include <Engine/Game/Component/Script.h>

#include <Engine/Core/CoreMinimal.h>

namespace sc
{
    class ControlScript :
        public engine::Script
    {
        ENTITY_INFO(ControlScript, engine::Script)
    public:
        ControlScript();
        virtual ~ControlScript() override;

        virtual void Awake() override;
        virtual void Update() override;

    private:
        void KeyDownDebug();
        void MousePosDebug();

    private:
        
    };
}