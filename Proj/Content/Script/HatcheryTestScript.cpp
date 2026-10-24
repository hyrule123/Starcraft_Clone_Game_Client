#include "Content/pch.h"
#include "HatcheryTestScript.h"

#include <Engine/Manager/InputManager.h>
#include <Engine/Game/GameObject.h>
#include <Engine/Core/Debug.h>

#include <Content/Scene/MainGameScene.h>

#include <SCSimulator/Include/SCSimulatorAPI.h>

namespace sc
{
	namespace
	{
		constexpr SCSim::Player kTestPlayer = SCSim::Player::_1;

		// 드론 생성 위치
		constexpr int32_t kDroneSpawnX = 3480;
		constexpr int32_t kDroneSpawnY = 3480;

		// 해처리(4×3 타일) 좌상단 타일. x 3552~3680, y 3424~3520, 중심 (3616, 3472)
		// 드론과 중심 거리 약 136px → 128px 이내로 들어온 뒤 그림자가 얼고 나머지 구간을 이동
		constexpr int32_t kHatcheryTileX = 105;
		constexpr int32_t kHatcheryTileY = 105;

		// 해처리 300. 반복 테스트를 위해 넉넉하게
		constexpr int32_t kTestMinerals = 1000;
	}

	HatcheryTestScript::HatcheryTestScript()
		: Super(HatcheryTestScript::kClassConcreteName)
	{}

	HatcheryTestScript::~HatcheryTestScript()
	{}

	void HatcheryTestScript::Awake()
	{
		Super::Awake();

		MainGameScene* scene = static_cast<MainGameScene*>(GetOwnerGameObject()->GetOwnerScene());
		ASSERT(scene);

		sim_ctx_ = scene->GetSCSimContext();
		CHECK(sim_ctx_);

		const bool resources_set = SCSim_Debug_SetResources(sim_ctx_, kTestPlayer, kTestMinerals, 0);
		CHECK_F(resources_set, "HatcheryTestScript: SCSim_Debug_SetResources failed: {}", SCSim_GetLastError());

		drone_id_ = SCSim_CreateUnit(sim_ctx_, SCSim::UnitType::ZergDrone, kTestPlayer, kDroneSpawnX, kDroneSpawnY);
		CHECK_F(drone_id_ != SCSim::kInvalidUnitId, "HatcheryTestScript: SCSim_CreateUnit failed: {}", SCSim_GetLastError());
	}

	void HatcheryTestScript::Update()
	{
		Super::Update();

		if (false == engine::InputManager::GetInst().GetKeyDown(engine::KeyCode::kH))
		{
			return;
		}

		// 다른 테스트 스크립트가 선택을 바꿨을 수 있으므로 명령 직전에 다시 선택 (SCSim_Build는 단일 선택 필수)
		const int32_t selected = SCSim_Select(sim_ctx_, kTestPlayer, &drone_id_, 1);
		if (selected != 1)
		{
			LOG("HatcheryTestScript: 드론 선택 실패: {}", SCSim_GetLastError());
			return;
		}

		// true = 명령 수락. 배치 불가·자원 부족이면 true여도 시뮬레이션이 무시함
		const bool ordered = SCSim_Build(sim_ctx_, kTestPlayer, SCSim::OrderType::DroneStartBuild,
			SCSim::UnitType::ZergHatchery, kHatcheryTileX, kHatcheryTileY);

		if (ordered)
		{
			LOG("HatcheryTestScript: 해처리 건설 명령 (tile {}, {})", kHatcheryTileX, kHatcheryTileY);
		}
		else
		{
			LOG("HatcheryTestScript: 해처리 건설 명령 실패: {}", SCSim_GetLastError());
		}
	}
}