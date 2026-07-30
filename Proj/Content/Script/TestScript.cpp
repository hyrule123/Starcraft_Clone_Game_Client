#include "Content/pch.h"
#include "TestScript.h"

#include <Engine/Core/Debug.h>

#include <Engine/Manager/InputManager.h>
#include <Engine/Game/Component/SpriteAnimator.h>
#include <Engine/Game/Scene.h>

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
		if (animator)
		{
			animator->AddNotifyCallback("MoveEnd"_hash, [this]() {
				DEBUG_LOG("TestScript received MoveEnd callback.");
				});
		}
	}

	void TestScript::OnEnable()
	{
		Super::OnEnable();

		DEBUG_LOG("TestScript OnEnable() called.");
	}

	void TestScript::Update()
	{
		Super::Update();
		
		if (InputManager::GetInst().GetKeyDown(KeyCode::kQ))
		{
			DEBUG_LOG("TestScript::Update() - Q key pressed.");

			if (target_enable_obj.expired())
			{
				DEBUG_LOG("Creating New TerranMarine Object...");

				target_enable_obj = GetOwnerGameObject()->GetOwnerScene()->AddGameObject("TerranMarine"_hash);
			}
			else
			{
				DEBUG_LOG("Destroying Target GameObject...");

				target_enable_obj.lock()->Destroy();
			}
		}

		if (auto target_obj = target_enable_obj.lock())
		{
			if (InputManager::GetInst().GetKeyDown(KeyCode::kSpace))
			{
				bool is_active = target_obj->IsActive();

				std::string msg = "TestScript::Update() - Toggling target GameObject active state: " + std::string((!is_active) ? "Active" : "Inactive");
				DEBUG_LOG_A(msg.c_str());

				target_obj->SetActive(!is_active);
			}
		}
	}
	void TestScript::OnDisable()
	{
		Super::OnDisable();
		DEBUG_LOG("TestScript OnDisable() called.");
	}
}


