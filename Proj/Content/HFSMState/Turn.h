#pragma once
#include <Engine/Game/Component/HFSMState.h>

#include <Engine/Core/CoreMinimal.h>

#include <Engine/Core/Math.h>

namespace sc
{
    class Turn :
        public engine::HFSMState
    {
		ENTITY_INFO(Turn, engine::HFSMState)
    public:
        Turn();
		virtual ~Turn() override;  

    private:
		float2* direction_ = {};
		float2* destination_ = {};
    };
}


