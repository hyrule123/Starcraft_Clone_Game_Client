#include "Content/pch.h"
#include "CameraMoveScript.h"

#include <Engine/Manager/InputManager.h>
#include <Engine/Manager/TimeManager.h>
#include <Engine/Core/EngineMain.h>

#include <Engine/Game/GameObject.h>
#include <Engine/Game/Component/Transform.h>
#include <Engine/Game/Component/Camera.h>

#include <Engine/Core/Debug.h>

#include <Content/SCData/Constants.h>

#include <Content/SCData/Image/SCSpriteCommon.h>

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
		if (is_move_locked_ == false)
		{
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

	int2 CameraMoveScript::ScreenToSc(int2 mouse_pos) const
	{
		const uint32_t screen_w = engine::EngineMain::GetInst().GetScreenWidth();
		const uint32_t screen_h = engine::EngineMain::GetInst().GetScreenHeight();
		ASSERT(screen_w > 0 && screen_h > 0);

		// 캡처 중 창 밖 좌표 → 클라이언트 영역으로 클램프 (원작은 커서가 화면 밖으로 나갈 수 없음)
		mouse_pos.x = std::clamp(mouse_pos.x, 0, static_cast<int32_t>(screen_w) - 1);
		mouse_pos.y = std::clamp(mouse_pos.y, 0, static_cast<int32_t>(screen_h) - 1);

		// 화면 픽셀 → SC 픽셀 비율 (Present가 kResolution을 창 전체로 늘리는 전제, 레터박스 없음)
		const float w_ratio = static_cast<float>(kResolutionWidth) / static_cast<float>(screen_w);
		const float h_ratio = static_cast<float>(kResolutionHeight) / static_cast<float>(screen_h);

		// SC 좌표계와 screen 좌표계(lt-rb)는 방향이 같음. 픽셀 인덱스이므로 내림
		// 캡처 중에는 창 밖(음수 포함) 좌표가 올 수 있으므로 정수 나눗셈 대신 floor 사용
		const float2 top_left = GetViewTopLeft();
		int32_t sc_x = static_cast<int32_t>(std::floor(top_left.x + (static_cast<float>(mouse_pos.x) * w_ratio)));
		int32_t sc_y = static_cast<int32_t>(std::floor(top_left.y + (static_cast<float>(mouse_pos.y) * h_ratio)));

		// 맵 범위로 클램프 (창 밖 좌표, 뷰보다 작은 맵의 바깥 영역). 범위 미설정 시 그대로
		if (sc_clamp_bounds_.IsValid())
		{
			sc_x = std::clamp(sc_x, static_cast<int32_t>(sc_clamp_bounds_.min.x), static_cast<int32_t>(sc_clamp_bounds_.max.x) - 1);
			sc_y = std::clamp(sc_y, static_cast<int32_t>(sc_clamp_bounds_.min.y), static_cast<int32_t>(sc_clamp_bounds_.max.y) - 1);
		}

		return int2{ sc_x, sc_y };
	}

	// 실제로 그려진 카메라 중심(Update와 동일하게 반올림) 기준 뷰 좌상단 (SC 좌표)
	float2 CameraMoveScript::GetViewTopLeft() const
	{
		return float2{ std::round(sc_cam_pos_.x) - (view_size_.x * 0.5f), std::round(sc_cam_pos_.y) - (view_size_.y * 0.5f) };
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