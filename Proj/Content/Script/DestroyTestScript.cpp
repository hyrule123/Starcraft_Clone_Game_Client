#include "Content/pch.h"
#include "DestroyTestScript.h"

#include <Engine/Manager/InputManager.h>
#include <Engine/Manager/TimeManager.h>

#include <Engine/Core/Debug.h>

namespace engine
{
	DestroyTestScript::DestroyTestScript()
		: Super(DestroyTestScript::kClassConcreteName)
	{}

	DestroyTestScript::~DestroyTestScript()
	{}
	void DestroyTestScript::Update()
	{
		Super::Update();

		if (InputManager::GetInst().GetKeyDown(KeyCode::kSpace))
		{
			has_test_started_ = true;
		}

		if (!has_test_started_) { return; }

		remain_time_ -= TimeManager::GetInst().DeltaTime();
		if ((int)prev_time_ != (int)remain_time_)
		{
			std::string msg = "DestroyTestScript::Update() - Remaining Time: " + std::to_string((int)remain_time_ + 1) + " seconds.";
			DEBUG_LOG_A(msg.c_str());
		}
		prev_time_ = remain_time_;

		if (remain_time_ <= 0.0f)
		{
			DEBUG_LOG("DestroyTestScript::Update() - Destroying GameObject.");
			GetOwnerGameObject()->Destroy();
		}
	}
	void DestroyTestScript::OnEnable()
	{
		Super::OnEnable();
		DEBUG_LOG("DestroyTestScript::OnEnable() called.");
	}
	void DestroyTestScript::OnDisable()
	{
		Super::OnDisable();
		DEBUG_LOG("DestroyTestScript::OnDisable() called.");
	}
	void DestroyTestScript::OnDestroy()
	{
		Super::OnDestroy();
		DEBUG_LOG("DestroyTestScript::OnDestroy() called.");
	}
}

