#pragma once
#include <Engine/Game/Component/Script.h>

#include <SCSimulator/Include/SCSimulatorTypes.h>

namespace sc
{
	// [테스트] 드론을 생성하고, H 키로 해처리 건설을 명령
	// - 그림자(modifier 10)의 y_frozen 확인용: 드론이 건설 위치 128px 이내에 들어오면 그림자가 얼고,
	//   도착할 때까지 드론만 이동함 (이동량이 커서 컬링 bounds 보정 확인에도 적합)
	// - Player 1의 선택을 공유하므로, 명령 직전에 항상 드론을 다시 선택함
	class HatcheryTestScript :
		public engine::Script
	{
		ENTITY_INFO(HatcheryTestScript, engine::Script);

	public:
		HatcheryTestScript();
		virtual ~HatcheryTestScript() override;

		virtual void Awake() override;
		virtual void Update() override;

	private:
		SCSim::Context* sim_ctx_ = nullptr;
		uint32_t drone_id_ = SCSim::kInvalidUnitId;
	};
}