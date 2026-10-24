#include "Content/pch.h"
#include "BuildTestScript.h"

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

		// 파일런(2×2 타일) 좌상단 타일. 중심 = 32 * tile + 32 = (512, 512)
		constexpr int32_t kPylonTileX = 15;
		constexpr int32_t kPylonTileY = 15;

		// 건설 위치(480~544)와 겹치지 않는 곳에 프로브 생성
		constexpr int32_t kProbeSpawnX = 440;
		constexpr int32_t kProbeSpawnY = 512;

		// 파일런 100. 반복 테스트를 위해 넉넉하게
		constexpr int32_t kTestMinerals = 1000;
	}

	BuildTestScript::BuildTestScript()
		: Super(BuildTestScript::kClassConcreteName)
	{}

	BuildTestScript::~BuildTestScript()
	{}

	void BuildTestScript::Awake()
	{
		Super::Awake();

		MainGameScene* scene = static_cast<MainGameScene*>(GetOwnerGameObject()->GetOwnerScene());
		ASSERT(scene);

		sim_ctx_ = scene->GetSCSimContext();
		CHECK(sim_ctx_);

		const bool resources_set = SCSim_Debug_SetResources(sim_ctx_, kTestPlayer, kTestMinerals, 0);
		CHECK_F(resources_set, "BuildTestScript: SCSim_Debug_SetResources failed: {}", SCSim_GetLastError());

		probe_id_ = SCSim_CreateUnit(sim_ctx_, SCSim::UnitType::ProtossProbe, kTestPlayer, kProbeSpawnX, kProbeSpawnY);
		CHECK_F(probe_id_ != SCSim::kInvalidUnitId, "BuildTestScript: SCSim_CreateUnit failed: {}", SCSim_GetLastError());
	}

	void BuildTestScript::Update()
	{
		Super::Update();

		if (false == engine::InputManager::GetInst().GetKeyDown(engine::KeyCode::kB))
		{
			return;
		}

		// 다른 테스트 스크립트가 선택을 바꿨을 수 있으므로 명령 직전에 다시 선택 (SCSim_Build는 단일 선택 필수)
		const int32_t selected = SCSim_Select(sim_ctx_, kTestPlayer, &probe_id_, 1);
		if (selected != 1)
		{
			LOG("BuildTestScript: 프로브 선택 실패: {}", SCSim_GetLastError());
			return;
		}

		// true = 명령 수락. 배치 불가·자원 부족이면 true여도 시뮬레이션이 무시함
		const bool ordered = SCSim_Build(sim_ctx_, kTestPlayer, SCSim::OrderType::PlaceProtossBuilding,
			SCSim::UnitType::ProtossPylon, kPylonTileX, kPylonTileY);

		if (ordered)
		{
			LOG("BuildTestScript: 파일런 건설 명령 (tile {}, {})", kPylonTileX, kPylonTileY);
		}
		else
		{
			LOG("BuildTestScript: 파일런 건설 명령 실패: {}", SCSim_GetLastError());
		}
	}
}