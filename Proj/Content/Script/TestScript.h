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
		virtual void Update() override;
	};
}


