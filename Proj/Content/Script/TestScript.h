#pragma once
#include <Engine/Game/Component/Script.h>

namespace engine
{
	class TestScript :
		public Script
	{
		CLASS_INFO(TestScript, Script)
	public:
		TestScript();
		virtual ~TestScript() override;

		virtual void BeginPlay() override;

		virtual void OnEnable() override;
		virtual void Update() override;
		virtual void OnDisable() override;

		void SetTargetEnableObject(const s_ptr<GameObject>& obj) { target_enable_obj = obj; }

	private:
		w_ptr<GameObject> target_enable_obj = {};
	};
}


