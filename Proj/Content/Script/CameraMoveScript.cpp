#include "Content/pch.h"
#include "CameraMoveScript.h"

#include <Engine/Manager/InputManager.h>
#include <Engine/Manager/TimeManager.h>

#include <Engine/Game/GameObject.h>
#include <Engine/Game/Component/Transform.h>

#include <Engine/Core/Debug.h>

namespace engine
{
	CameraMoveScript::CameraMoveScript()
		: Super(CameraMoveScript::kClassConcreteName)
		, speed_(500.0f)
		, pos_update_interval_(1.0f)
	{}
	CameraMoveScript::~CameraMoveScript()
	{}

	void CameraMoveScript::Update()
	{
		Super::Update();
		Transform* transform = GetComponent<Transform>();
		ASSERT(transform);
		float3 pos = transform->GetLocalPosition();
		const float delta_time = TimeManager::GetInst().GetDeltaTime();
		float2 move_dir = float2::Zero;
		if (InputManager::GetInst().GetKey(KeyCode::kUp)) { move_dir.y += 1.0f; }
		if (InputManager::GetInst().GetKey(KeyCode::kDown)) { move_dir.y -= 1.0f; }
		if (InputManager::GetInst().GetKey(KeyCode::kLeft)) { move_dir.x -= 1.0f; }
		if (InputManager::GetInst().GetKey(KeyCode::kRight)) { move_dir.x += 1.0f; }
		if (move_dir != float2::Zero)
		{
			move_dir.Normalize();
			
			pos += float3(move_dir * speed_ * delta_time, 0.0f);
			
			transform->SetLocalPosition(pos);
		}

		pos_update_timer_ += delta_time;
		if(pos_update_timer_ >= pos_update_interval_)
		{
			pos_update_timer_ = 0.0f;
			DEBUG_LOG("Camera Position: {}, {}, {}", pos.x, pos.y, pos.z);
		}
	}
}