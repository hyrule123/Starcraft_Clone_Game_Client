#pragma once
#include <Engine/Game/Component/Script.h>

namespace engine
{
    class ActiveEnableTest :
        public Script
    {
		ENTITY_INFO(ActiveEnableTest, Script)

    public:
        ActiveEnableTest();
		virtual ~ActiveEnableTest() override;

		virtual void OnEnable() override;
		virtual void Update() override;
		virtual void OnDisable() override;

    private:
		float remain_time_ = 3.0f;
		float prev_time_ = 3.0f;
		bool has_test_started_ = false;
    };
}
