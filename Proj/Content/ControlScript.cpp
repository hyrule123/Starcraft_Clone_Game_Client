#include "Content/pch.h"
#include "ControlScript.h"

#include <Engine/Manager/InputManager.h>

#include <Engine/Core/Debug.h>

namespace engine
{
	ControlScript::ControlScript()
		: Super(STRINGIFY(ControlScript))
	{}

	ControlScript::~ControlScript()
	{
		
	}
	void ControlScript::Update()
	{
		Super::Update();

		const auto& down_keys = InputManager::GetInst().GetDownKeys();
		const auto& pressed_keys = InputManager::GetInst().GetPressedKeys();
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
				msg	+= " Key Up!";
				DEBUG_LOG_A(msg.c_str());
			}
		}
	}
}

