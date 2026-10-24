#pragma once
#include <Engine/Game/Component/Script.h>

#include <SCSimulator/Include/SCSimulatorTypes.h>

namespace sc
{
	// [테스트] SCV를 생성하고, V 키로 커맨드 센터 건설, L 키로 이륙/착륙을 토글
	// - 그림자(modifier 10)의 y_frozen 확인용: 이·착륙 중 그림자가 지면에 머무는지
	// - 착륙은 원래 자리(같은 타일)로 내림
	// - Player 1의 선택을 공유하므로, 명령 직전에 항상 대상을 다시 선택함
	class LiftoffTestScript :
		public engine::Script
	{
		ENTITY_INFO(LiftoffTestScript, engine::Script);

	public:
		LiftoffTestScript();
		virtual ~LiftoffTestScript() override;

		virtual void Awake() override;
		virtual void Update() override;

	private:
		void OrderBuild();
		void ToggleLiftoff();

		// 커맨드 센터의 현재 스냅샷을 찾음. 못 찾으면 false (out 상태는 미정)
		// - 처음엔 유닛 타입 + 건설 위치로 찾아 cc_id_에 캐시, 이후엔 cc_id_로 찾음(이륙 후 위치가 바뀌므로)
		bool FindCommandCenter(SCSim::UnitData& out);

		SCSim::Context* sim_ctx_ = nullptr;
		uint32_t scv_id_ = SCSim::kInvalidUnitId;
		uint32_t cc_id_ = SCSim::kInvalidUnitId;
	};
}