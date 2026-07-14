#pragma once
#include <Engine/Game/Component/HFSMState.h>

#include <Engine/Core/CoreMinimal.h>

namespace engine
{
    class Locomotion :
        public HFSMState
    {
        CLASS_INFO(Locomotion, HFSMState)
	public:
        Locomotion();
		virtual ~Locomotion() override;

		virtual void OnEnter(const AIContext& ai_context) override;
    };
}


