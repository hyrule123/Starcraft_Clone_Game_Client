#include "Content/pch.h"
#include "LiftoffTestScript.h"

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

		// 커맨드 센터(4×3 타일) 좌상단 타일. x 3520~3648, y 352~448
		constexpr int32_t kCCTileX = 110;
		constexpr int32_t kCCTileY = 11;

		// 건물 위치(unit_t::position) = 좌상단 타일 + 배치 크기 / 2 = (3584, 400)
		constexpr int32_t kCCCenterX = kCCTileX * 32 + (4 * 32) / 2;
		constexpr int32_t kCCCenterY = kCCTileY * 32 + (3 * 32) / 2;

		// 건설 영역(x 3520~)과 겹치지 않는 곳에 SCV 생성
		constexpr int32_t kScvSpawnX = 3480;
		constexpr int32_t kScvSpawnY = 400;

		// 커맨드 센터 400. 반복 테스트를 위해 넉넉하게
		constexpr int32_t kTestMinerals = 1000;
	}

	LiftoffTestScript::LiftoffTestScript()
		: Super(LiftoffTestScript::kClassConcreteName)
	{}

	LiftoffTestScript::~LiftoffTestScript()
	{}

	void LiftoffTestScript::Awake()
	{
		Super::Awake();

		MainGameScene* scene = static_cast<MainGameScene*>(GetOwnerGameObject()->GetOwnerScene());
		ASSERT(scene);

		sim_ctx_ = scene->GetSCSimContext();
		CHECK(sim_ctx_);

		const bool resources_set = SCSim_Debug_SetResources(sim_ctx_, kTestPlayer, kTestMinerals, 0);
		CHECK_F(resources_set, "LiftoffTestScript: SCSim_Debug_SetResources failed: {}", SCSim_GetLastError());

		scv_id_ = SCSim_CreateUnit(sim_ctx_, SCSim::UnitType::TerranSCV, kTestPlayer, kScvSpawnX, kScvSpawnY);
		CHECK_F(scv_id_ != SCSim::kInvalidUnitId, "LiftoffTestScript: SCSim_CreateUnit failed: {}", SCSim_GetLastError());
	}

	void LiftoffTestScript::Update()
	{
		Super::Update();

		engine::InputManager& input = engine::InputManager::GetInst();

		if (input.GetKeyDown(engine::KeyCode::kV))
		{
			OrderBuild();
		}

		if (input.GetKeyDown(engine::KeyCode::kL))
		{
			ToggleLiftoff();
		}
	}

	void LiftoffTestScript::OrderBuild()
	{
		// 다른 테스트 스크립트가 선택을 바꿨을 수 있으므로 명령 직전에 다시 선택 (SCSim_Build는 단일 선택 필수)
		const int32_t selected = SCSim_Select(sim_ctx_, kTestPlayer, &scv_id_, 1);
		if (selected != 1)
		{
			LOG("LiftoffTestScript: SCV 선택 실패: {}", SCSim_GetLastError());
			return;
		}

		// true = 명령 수락. 배치 불가·자원 부족이면 true여도 시뮬레이션이 무시함
		const bool ordered = SCSim_Build(sim_ctx_, kTestPlayer, SCSim::OrderType::PlaceBuilding,
			SCSim::UnitType::TerranCommandCenter, kCCTileX, kCCTileY);

		if (ordered)
		{
			LOG("LiftoffTestScript: 커맨드 센터 건설 명령 (tile {}, {})", kCCTileX, kCCTileY);
		}
		else
		{
			LOG("LiftoffTestScript: 커맨드 센터 건설 명령 실패: {}", SCSim_GetLastError());
		}
	}

	void LiftoffTestScript::ToggleLiftoff()
	{
		SCSim::UnitData cc = {};
		if (false == FindCommandCenter(cc))
		{
			LOG("LiftoffTestScript: 커맨드 센터 없음 (V로 먼저 건설)");
			return;
		}

		if (0 == (cc.status_flags & SCSim::UnitStatusFlags::Completed))
		{
			LOG("LiftoffTestScript: 커맨드 센터 건설 중");
			return;
		}

		const int32_t selected = SCSim_Select(sim_ctx_, kTestPlayer, &cc_id_, 1);
		if (selected != 1)
		{
			LOG("LiftoffTestScript: 커맨드 센터 선택 실패: {}", SCSim_GetLastError());
			return;
		}

		if (cc.status_flags & SCSim::UnitStatusFlags::GroundedBuilding)
		{
			const bool ordered = SCSim_Liftoff(sim_ctx_, kTestPlayer);
			if (ordered)
			{
				LOG("LiftoffTestScript: 이륙 명령");
			}
			else
			{
				LOG("LiftoffTestScript: 이륙 명령 실패: {}", SCSim_GetLastError());
			}
		}
		else
		{
			// 원래 자리로 착륙. BuildingLand는 unit_type이 선택 유닛 자신의 타입이어야 함
			const bool ordered = SCSim_Build(sim_ctx_, kTestPlayer, SCSim::OrderType::BuildingLand,
				SCSim::UnitType::TerranCommandCenter, kCCTileX, kCCTileY);
			if (ordered)
			{
				LOG("LiftoffTestScript: 착륙 명령 (tile {}, {})", kCCTileX, kCCTileY);
			}
			else
			{
				LOG("LiftoffTestScript: 착륙 명령 실패: {}", SCSim_GetLastError());
			}
		}
	}

	bool LiftoffTestScript::FindCommandCenter(SCSim::UnitData& out)
	{
		const SCSim::UnitData* units = nullptr;
		const int32_t count = SCSim_GetUnits(sim_ctx_, &units);
		if (count < 0)
		{
			LOG("LiftoffTestScript: SCSim_GetUnits 실패: {}", SCSim_GetLastError());
			return false;
		}

		for (int32_t i = 0; i < count; ++i)
		{
			const SCSim::UnitData& u = units[i];

			const bool matched = (cc_id_ != SCSim::kInvalidUnitId)
				? (u.id == cc_id_)
				// 맵에 다른 커맨드 센터가 있을 수 있으므로 건설 위치까지 비교
				: (u.unit_type == SCSim::UnitType::TerranCommandCenter && u.x == kCCCenterX && u.y == kCCCenterY);

			if (matched)
			{
				cc_id_ = u.id;
				out = u;
				return true;
			}
		}

		// 캐시된 커맨드 센터가 죽었으면 다음에 다시 위치로 찾음
		cc_id_ = SCSim::kInvalidUnitId;
		return false;
	}
}