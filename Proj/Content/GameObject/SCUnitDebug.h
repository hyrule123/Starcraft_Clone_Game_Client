#pragma once
#include <Engine/Game/GameObject.h>

namespace engine
{
    class SCUnitDebug :
        public GameObject
    {
        ENTITY_INFO(SCUnitDebug, GameObject);

	public:
        SCUnitDebug();
        virtual ~SCUnitDebug() override;

        virtual void Init() override;

    private:

    };
}


