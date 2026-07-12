#include "Content/pch.h"
#include "ControlScript.h"

#include <Engine/Manager/InputManager.h>
#include <Engine/Manager/TimeManager.h>

#include <Engine/Core/Debug.h>

#include <Engine/Game/Component/Transform.h>

namespace engine
{
	ControlScript::ControlScript()
		: Super(STRINGIFY(ControlScript))
	{}

	ControlScript::~ControlScript()
	{
		
	}
	void ControlScript::Awake()
	{
		Super::Awake();

		s_ptr<Transform> tf = GetComponent<Transform>();
		tf->SetLocalPosition(float3(0.0f, 0.0f, 1.5f));
		tf->SetLocalScale(float3(100.0f, 100.0f, 1.0f));
	}
	void ControlScript::Update()
	{
		Super::Update();

		InputManager& input = InputManager::GetInst();
		float dt = TimeManager::GetInst().DeltaTime();
		
		s_ptr<Transform> tr = GetComponent<Transform>();
		float3 pos = tr->GetLocalPosition();

		if (input.GetKey(KeyCode::kUp))
		{
			pos.y += dt * 100.0f;
		}
		else if (input.GetKey(KeyCode::kDown))
		{
			pos.y -= dt * 100.0f;
		}

		if (input.GetKey(KeyCode::kLeft))
		{
			pos.x -= dt * 100.0f;
		}
		else if (input.GetKey(KeyCode::kRight))
		{
			pos.x += dt * 100.0f;
		}

		tr->SetLocalPosition(pos);


		int a = 3;
		//input.GetKey()


		//KeyDownDebug();
		//MousePosDebug();
	}
	void ControlScript::KeyDownDebug()
	{
		const auto& down_keys = InputManager::GetInst().GetDownKeys();
		const auto& pressed_keys = InputManager::GetInst().GetKeys();
		const auto& up_keys = InputManager::GetInst().GetUpKeys();

		for (size_t i = 0; i < down_keys.size(); ++i)
		{
			if (down_keys[i])
			{
				std::string msg;
				msg += kKeyCodeNames[i];
				msg += " Key Down!";
				DEBUG_LOG_A(msg.c_str());
			}

			if (pressed_keys[i])
			{
				std::string msg;
				msg += kKeyCodeNames[i];
				msg += " Key Pressed!";
				DEBUG_LOG_A(msg.c_str());
			}

			if (up_keys[i])
			{
				std::string msg;
				msg += kKeyCodeNames[i];
				msg += " Key Up!";
				DEBUG_LOG_A(msg.c_str());
			}
		}
	}
	void ControlScript::MousePosDebug()
	{
		int32_2 mousepos = InputManager::GetInst().GetMousePos();
		std::string msg = "Mouse Pos: x(";
		msg += std::to_string(mousepos.x) + ") (";
		msg += std::to_string(mousepos.y) + ")";
		DEBUG_LOG_A(msg.c_str());
	}
}

