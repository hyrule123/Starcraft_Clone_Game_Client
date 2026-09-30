#pragma once
#include <Engine/Game/Component/Script.h>

namespace sc
{
    class DestroyTestScript :
        public engine::Script
    {
		ENTITY_INFO(DestroyTestScript, engine::Script)
	public:
		DestroyTestScript();
		virtual ~DestroyTestScript() override;

		virtual void Update() override;

		virtual void OnEnable() override;
		virtual void OnDisable() override;

		virtual void OnDestroy() override;

	private:
		float remain_time_ = 3.0f;
		float prev_time_ = 3.0f;

		bool has_test_started_ = false;
    };
}


