#include "Content/pch.h"
#include "TestScript.h"

#include <Engine/Core/Debug.h>

#include <Engine/Game/Component/SpriteAnimator.h>

namespace engine
{
	TestScript::TestScript()
		: Super(TestScript::kClassConcreteName)
	{}

	TestScript::~TestScript()
	{}

	void TestScript::BeginPlay()
	{
		Super::BeginPlay();

		DEBUG_LOG("TestScript BeginPlay() called.");

		auto animator = GetComponent<SpriteAnimator>();
		animator->AddNotifyCallback("MoveEnd"_hash, [this]() {
			DEBUG_LOG("TestScript received MoveEnd callback.");
			});
	}

	void TestScript::Update()
	{
		Super::Update();
		
	}
}


