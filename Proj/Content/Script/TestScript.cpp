#include "Content/pch.h"
#include "TestScript.h"

#include <Engine/Core/Debug.h>
#include <Engine/Core/Constant.h>

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

			if (target_obj_.expired())
			{
				DEBUG_LOG("Creating New TerranMarine Object...");

				target_obj_ = GetOwnerGameObject()->GetOwnerScene()->AddGameObject("TerranMarine"_hash);
			}
			else
			{
				DEBUG_LOG("Destroying Target GameObject...");

				target_obj_.lock()->Destroy();
			}
		}

		if (auto target_obj = target_obj_.lock())
		{
			if (InputManager::GetInst().GetKeyDown(KeyCode::kSpace))
			{
				bool is_active = target_obj->IsActive();

				std::string msg = "TestScript::Update() - Toggling target GameObject active state: " + std::string((!is_active) ? "Active" : "Inactive");
				DEBUG_LOG_A(msg.c_str());

				target_obj->SetActive(!is_active);
			}


			if (InputManager::GetInst().GetKeyDown(KeyCode::kC))
			{
				static uint32 layer = 0;
				layer = (target_obj->GetLayer() + 1) % kLayerMaxCount;
				target_obj->SetLayer(layer);

				std::string msg = "TestScript::Update() - Changing target GameObject layer to: " + std::to_string(layer);
				DEBUG_LOG_A(msg.c_str());
			}
		}


	}
	void TestScript::OnDisable()
	{
		Super::OnDisable();
		DEBUG_LOG("TestScript OnDisable() called.");
	}
}


