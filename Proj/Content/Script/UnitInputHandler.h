#pragma once
#include <Engine/Game/Component/Script.h>

#include <Engine/Core/CoreMinimal.h>
#include <Engine/Core/Math.h>

namespace engine
{
    class InputManager;
    class Camera;
    class BlackBoard;

    class UnitInputHandler :
        public Script
    {
		ENTITY_INFO(UnitInputHandler, Script)
    public:
        UnitInputHandler();
		virtual ~UnitInputHandler() override;

        virtual void Awake() override;
		virtual void Update() override;

    private:
        InputManager& input_mgr_;
		BlackBoard* blackboard_ = {};

		HashedStringView* command_input_ = {};
		float2* destination_ = {};
    };
}

