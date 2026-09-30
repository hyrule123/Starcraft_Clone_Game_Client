#pragma once
#include <Engine/Game/Component/HFSMState.h>

#include <Engine/Core/CoreMinimal.h>

#include <Engine/Core/Math.h>

namespace engine
{
    class BlackBoard;
}

namespace sc
{
    class Locomotion :
        public engine::HFSMState
    {
        ENTITY_INFO(Locomotion, engine::HFSMState)
    public:
        Locomotion();
        virtual ~Locomotion() override;

		virtual void OnAwake(const engine::AIContext& ai_context) override;
        virtual void OnEnter(const engine::AIContext& ai_context) override;

        virtual engine::HashedStringView CheckTransition(const engine::AIContext& ai_context) override;

    private:
        float2* direction_ = {};
		float2* destination_ = {};
		float* speed_ = {};
    };
}


