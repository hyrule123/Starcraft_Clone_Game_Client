#include "Content/pch.h"
#include "CameraMoveScript.h"

#include <Engine/Manager/InputManager.h>
#include <Engine/Manager/TimeManager.h>

#include <Engine/Game/GameObject.h>
#include <Engine/Game/Component/Transform.h>
#include <Engine/Game/Component/Camera.h>

#include <Engine/Core/Debug.h>

#include <Content/SCData/Constants.h>

#include <Content/SCData/SCSpriteCommon.h>

namespace sc
{
	CameraMoveScript::CameraMoveScript()
		: Super(CameraMoveScript::kClassConcreteName)
		, speed_(500.0f)
		, pos_update_interval_(1.0f)
		, view_size_{ static_cast<float>(sc::kResolutionWidth), static_cast<float>(sc::kResolutionHeight) }
	{}
	CameraMoveScript::~CameraMoveScript()
	{}

	void CameraMoveScript::Awake()
	{
		Super::Awake();

		engine::Camera* cam = GetComponent<engine::Camera>();
		ASSERT(cam);
		cam_ = cam;
	}

	void CameraMoveScript::Update()
	{
		using namespace engine;

		Super::Update();

		const float delta_time = TimeManager::GetInst().GetDeltaTime();
		float2 move_dir = float2::Zero;
		if (InputManager::GetInst().GetKey(KeyCode::kUp)) { move_dir.y -= 1.0f; }
		if (InputManager::GetInst().GetKey(KeyCode::kDown)) { move_dir.y += 1.0f; }
		if (InputManager::GetInst().GetKey(KeyCode::kLeft)) { move_dir.x -= 1.0f; }
		if (InputManager::GetInst().GetKey(KeyCode::kRight)) { move_dir.x += 1.0f; }
		if (move_dir != float2::Zero)
		{
			move_dir.Normalize();
			
			sc_cam_pos_ += float2(move_dir * speed_ * delta_time);
		}
		ClampCameraPosition();

		Transform* tr = GetComponent<Transform>();
		ASSERT(tr);
		const float3 local_pos = tr->GetLocalPosition();
		const float2 dx_cam_pos = ScToWorld(std::round(sc_cam_pos_.x), std::round(sc_cam_pos_.y));
		tr->SetLocalPosition(float3(dx_cam_pos.x, dx_cam_pos.y, local_pos.z));

		pos_update_timer_ += delta_time;
		if(pos_update_timer_ >= pos_update_interval_)
		{
			pos_update_timer_ = 0.0f;
			DEBUG_LOG("Camera Position: {}, {}, {}", dx_cam_pos.x, dx_cam_pos.y, local_pos.z);
		}
	}
	void CameraMoveScript::ClampCameraPosition()
	{
		using namespace engine;

		if (sc_clamp_bounds_.IsValid() == false) { return; }

		const float2 bounds_size = sc_clamp_bounds_.GetSize();
		if (bounds_size.x < view_size_.x)
		{
			sc_cam_pos_.x = sc_clamp_bounds_.min.x + (bounds_size.x * 0.5f);
		}
		else
		{
			const float l = sc_clamp_bounds_.min.x + (view_size_.x * 0.5f);
			const float r = sc_clamp_bounds_.max.x - (view_size_.x * 0.5f);
			sc_cam_pos_.x = std::clamp(sc_cam_pos_.x, l, r);
		}

		if (bounds_size.y < view_size_.y)
		{
			sc_cam_pos_.y = sc_clamp_bounds_.min.y + (bounds_size.y * 0.5f);
		}
		else
		{
			const float t = sc_clamp_bounds_.min.y + (view_size_.y * 0.5f);
			const float b = sc_clamp_bounds_.max.y - (view_size_.y * 0.5f);
			sc_cam_pos_.y = std::clamp(sc_cam_pos_.y, t, b);
		}
	}
}