#pragma once
#include <Engine/Game/Component/HFSMState.h>

#include <Engine/Core/CoreMinimal.h>

#include <Engine/Core/Math.h>

namespace engine
{
    class Turn :
        public HFSMState
    {
		CLASS_INFO(Turn, HFSMState)
    public:
        Turn();
		virtual ~Turn() override;  

    private:
		float2* direction_ = {};
		float2* destination_ = {};
    };
}


