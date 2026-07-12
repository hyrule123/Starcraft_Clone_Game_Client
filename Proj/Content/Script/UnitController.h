#pragma once
#include <Engine/Game/Component/Script.h>

#include <Engine/Core/CoreMinimal.h>

#include <Engine/Core/Math.h>

namespace engine
{
    class UnitController :
        public Script
    {
        CLASS_INFO(UnitController, Script)
	public:
		UnitController();
		virtual ~UnitController() override;

    private:
		float2 direction_ = {};

    };
}


