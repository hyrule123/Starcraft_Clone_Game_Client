#pragma once
#include <Engine/Game/Component/Script.h>

namespace engine
{
    class ActiveEnableTest :
        public Script
    {
		CLASS_INFO(ActiveEnableTest, Script)

    public:
        ActiveEnableTest();
		virtual ~ActiveEnableTest() override;

		virtual void Update() override;

    private:
		float remain_time_ = 3.0f;
		float prev_time_ = 3.0f;
		bool has_test_started_ = false;
    };
}
