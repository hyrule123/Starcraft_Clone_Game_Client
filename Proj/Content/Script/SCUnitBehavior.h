#pragma once
#include <Engine/Game/Component/Component.h>

namespace engine
{
    class SCUnitBehavior :
        public Component
    {
		ENTITY_INFO(SCUnitBehavior, Component);
		COMPONENT_CATEGORY(ComponentCategory::kBehavior);

    public:
        SCUnitBehavior();
        virtual ~SCUnitBehavior() override;
    };
}


