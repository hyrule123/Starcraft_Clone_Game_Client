#pragma once
#include <Engine/Game/Component/Script.h>

#include <SCSimulator/Include/SCSimulatorTypes.h>

namespace sc
{
	// [테스트] 프로브를 생성하고, B 키로 파일런 건설을 명령
	// - 건설 시간이 끝나면 WarpTexture(12) → WarpFlash(17) → 완성 순서로 진행됨
	// - Player 1의 선택을 공유하므로, 명령 직전에 항상 프로브를 다시 선택함
	class BuildTestScript :
		public engine::Script
	{
		ENTITY_INFO(BuildTestScript, engine::Script);

	public:
		BuildTestScript();
		virtual ~BuildTestScript() override;

		virtual void Awake() override;
		virtual void Update() override;

	private:
		SCSim::Context* sim_ctx_ = nullptr;
		uint32_t probe_id_ = SCSim::kInvalidUnitId;
	};
}