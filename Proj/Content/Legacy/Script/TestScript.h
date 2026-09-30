#pragma once
#include <Engine/Game/Component/Script.h>

namespace sc
{
	class TestScript :
		public engine::Script
	{
		ENTITY_INFO(TestScript, engine::Script)
	public:
		TestScript();
		virtual ~TestScript() override;

		virtual void Awake() override;

		virtual void OnEnable() override;
		virtual void Update() override;
		virtual void OnDisable() override;

		void SetTargetEnableObject(engine::weak_handle_ptr<engine::GameObject> obj) { target_obj_ = obj; }

	private:
		engine::weak_handle_ptr<engine::GameObject> target_obj_ = {};
	};
}


