#pragma once
#include <Engine/Game/Component/Script.h>

namespace engine
{
	class TestScript :
		public Script
	{
		ENTITY_INFO(TestScript, Script)
	public:
		TestScript();
		virtual ~TestScript() override;

		virtual void BeginPlay() override;

		virtual void OnEnable() override;
		virtual void Update() override;
		virtual void OnDisable() override;

		void SetTargetEnableObject(wh_ptr<GameObject> obj) { target_obj_ = obj; }

	private:
		wh_ptr<GameObject> target_obj_ = {};
	};
}


