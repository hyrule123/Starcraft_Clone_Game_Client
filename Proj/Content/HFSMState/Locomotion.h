#pragma once
#include <Engine/Game/Component/HFSMState.h>

#include <Engine/Core/CoreMinimal.h>

#include <Engine/Core/Math.h>

namespace engine
{
    class BlackBoard;
    class Locomotion :
        public HFSMState
    {
        ENTITY_INFO(Locomotion, HFSMState)
    public:
        Locomotion();
        virtual ~Locomotion() override;

		virtual void OnAwake(const AIContext& ai_context) override;
        virtual void OnEnter(const AIContext& ai_context) override;

        virtual HashedStringView CheckTransition(const AIContext& ai_context) override;

    private:
        float2* direction_ = {};
		float2* destination_ = {};
		float* speed_ = {};
    };
}


