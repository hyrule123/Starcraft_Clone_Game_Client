#pragma once
#include <Engine/Game/Component/Script.h>

namespace sc
{
    class ActiveEnableTest :
        public engine::Script
    {
		ENTITY_INFO(ActiveEnableTest, engine::Script)

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
