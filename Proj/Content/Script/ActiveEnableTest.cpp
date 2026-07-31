#include "Content/pch.h"
#include "ActiveEnableTest.h"

#include <Engine/Manager/InputManager.h>
#include <Engine/Manager/TimeManager.h>
#include <Engine/Core/Debug.h>

namespace engine
{
	ActiveEnableTest::ActiveEnableTest()
		: Super(ActiveEnableTest::kClassConcreteName)
	{}
	ActiveEnableTest::~ActiveEnableTest()
	{}
	void ActiveEnableTest::OnEnable()
	{
		Super::OnEnable();
		DEBUG_LOG("ActiveEnableTest::OnEnable() Called!");
	}
	void ActiveEnableTest::Update()
	{
		Super::Update();

		if (InputManager::GetInst().GetKeyDown(KeyCode::kSpace))
		{
			has_test_started_ = true;
		}

		if (!has_test_started_) { return; }

		remain_time_ -= TimeManager::GetInst().GetDeltaTime();
		if ((int)prev_time_ != (int)remain_time_)
		{
			std::string msg = "ActiveEnableTest::Update() - Remaining Time: " + std::to_string((int)remain_time_ + 1) + " seconds.";
			DEBUG_LOG_A(msg.c_str());
		}
		prev_time_ = remain_time_;

		if (remain_time_ <= 0.0f)
		{
			DEBUG_LOG("ActiveEnableTest::Update() - Inactivating GameObject.");
			GetOwnerGameObject()->SetActive(false);
		}
	}
	void ActiveEnableTest::OnDisable()
	{
		Super::OnDisable();
		DEBUG_LOG("ActiveEnableTest::OnDisable() Called!");
	}
}