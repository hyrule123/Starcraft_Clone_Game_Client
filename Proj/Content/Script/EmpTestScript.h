#pragma once
#include <Engine/Game/Component/Script.h>

#include <SCSimulator/Include/SCSimulatorTypes.h>

namespace sc
{
	// [테스트] HeroMagellan을 생성·선택하고, E 키로 EMP Shockwave를 사용
	// - 영웅 유닛이라 EMP 연구가 필요 없음
	// - 생성 시 에너지는 최대치의 1/4(62.5). EMP(100)를 쓰려면 약 1,200 시뮬레이션 프레임(약 50초) 대기
	class EmpTestScript :
		public engine::Script
	{
		ENTITY_INFO(EmpTestScript, engine::Script);

	public:
		EmpTestScript();
		virtual ~EmpTestScript() override;

		virtual void Awake() override;
		virtual void Update() override;

	private:
		SCSim::Context* sim_ctx_ = nullptr;
		uint32_t vessel_id_ = SCSim::kInvalidUnitId;
	};
}