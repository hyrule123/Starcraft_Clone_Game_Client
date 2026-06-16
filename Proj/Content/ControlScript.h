#pragma once
#include <Engine/Game/Component/Script.h>

#include <Engine/Core/CoreMinimal.h>

namespace engine
{
    class ControlScript :
        public Script
    {
        CLASS_INFO(ControlScript, Script);
    public:
        ControlScript();
        virtual ~ControlScript() override;

        virtual void Update() override;

    private:
        
    };
}