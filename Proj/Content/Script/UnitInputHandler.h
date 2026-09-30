#pragma once
#include <Engine/Game/Component/Script.h>

#include <Engine/Core/CoreMinimal.h>
#include <Engine/Core/Math.h>

namespace engine
{
    class InputManager;
    class Camera;
    class BlackBoard;
}

namespace sc
{
    class UnitInputHandler :
        public engine::Script
    {
		ENTITY_INFO(UnitInputHandler, engine::Script)
    public:
        UnitInputHandler();
		virtual ~UnitInputHandler() override;

        virtual void Awake() override;
		virtual void Update() override;

    private:
        engine::InputManager& input_mgr_;
		engine::BlackBoard* blackboard_ = {};

		engine::HashedStringView* command_input_ = {};
		float2* destination_ = {};
    };
}

