#pragma once
#include <Engine/Game/Component/Component.h>

namespace sc
{
    class SCUnitBehavior :
        public engine::Component
    {
		ENTITY_INFO(SCUnitBehavior, engine::Component);
		COMPONENT_CATEGORY(engine::ComponentCategory::kBehavior);

    public:
        SCUnitBehavior();
        virtual ~SCUnitBehavior() override;
    };
}


