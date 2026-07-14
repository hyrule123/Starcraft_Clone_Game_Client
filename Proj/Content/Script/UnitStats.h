#pragma once
#include <Engine/Game/Component/Script.h>

#include <Engine/Core/CoreMinimal.h>

namespace engine
{
    class UnitStats :
        public Script
    {
		CLASS_INFO(UnitStats, Script)
    public:
        UnitStats();
		virtual ~UnitStats() override;

		void SetName(const std::string_view name) { name_ = name; }
		void SetMovementSpeed(float speed) { movement_speed_ = speed; }

    private:
		std::string name_ = {};
		float movement_speed_ = {};
    };

}

