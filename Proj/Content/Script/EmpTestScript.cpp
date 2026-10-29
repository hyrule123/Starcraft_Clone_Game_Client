#include "Content/pch.h"
#include "EmpTestScript.h"

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

		// 기존 테스트 유닛(마린·레이스·다크 템플러)이 모여 있는 위치
		constexpr int32_t kTargetX = 2048;
		constexpr int32_t kTargetY = 2048;

		// EMP 사거리(8타일 = 256px) 안쪽에 생성
		constexpr int32_t kSpawnX = kTargetX - 160;
		constexpr int32_t kSpawnY = kTargetY;
	}

	EmpTestScript::EmpTestScript()
		: Super(EmpTestScript::kClassConcreteName)
	{}

	EmpTestScript::~EmpTestScript()
	{}

	void EmpTestScript::Awake()
	{
		Super::Awake();

		MainGameScene* scene = static_cast<MainGameScene*>(GetOwnerGameObject()->GetOwnerScene());
		ASSERT(scene);

		sim_ctx_ = scene->GetSCSimContext();
		CHECK(sim_ctx_);

		vessel_id_ = SCSim_CreateUnit(sim_ctx_, SCSim::UnitType::HeroMagellan, kTestPlayer, kSpawnX, kSpawnY);
		CHECK_F(vessel_id_ != SCSim::kInvalidUnitId, "EmpTestScript: SCSim_CreateUnit failed: {}", SCSim_GetLastError());

		//const int32_t selected = SCSim_Select(sim_ctx_, kTestPlayer, &vessel_id_, 1);
		//CHECK_F(selected == 1, "EmpTestScript: SCSim_Select failed: {}", SCSim_GetLastError());
	}

	void EmpTestScript::Update()
	{
		Super::Update();

		if (false == engine::InputManager::GetInst().GetKeyDown(engine::KeyCode::kE))
		{
			return;
		}

		int32_t selected = SCSim_Select(sim_ctx_, kTestPlayer, &vessel_id_, 1);
		if (selected != 1)
		{
			LOG("EmpTestScript: 시전할 유닛 선택 실패: {}", SCSim_GetLastError());
			return;
		}

		// 에너지가 부족해도 명령 자체는 수락될 수 있음 (시전 시점에 실패) → 반응이 없으면 에너지 대기
		const bool ordered = SCSim_Order(sim_ctx_, kTestPlayer, SCSim::OrderType::CastEMPShockwave,
			kTargetX, kTargetY, SCSim::kInvalidUnitId, SCSim::UnitType::None, false);

		if (ordered)
		{
			LOG("EmpTestScript: EMP 명령 ({}, {})", kTargetX, kTargetY);
		}
		else
		{
			LOG("EmpTestScript: EMP 명령 실패: {}", SCSim_GetLastError());
		}
	}
}