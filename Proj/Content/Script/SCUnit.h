#pragma once
#include <Engine/Game/Component/Script.h>

#include <SCSimulator/Include/SCSimulatorAPI.h>

//클래스 설명: SCUnit의 main 컴포넌트(모든 데이터 저장)
namespace sc
{
    class SCUnit :
        public engine::Script
    {
        ENTITY_INFO(SCUnit, engine::Script);

	public:
        SCUnit();
        virtual ~SCUnit() override;

		void SetSnapshot(const SCSim::UnitData& snapshot) { snapshot_ = snapshot; }
		const SCSim::UnitData& GetSnapshot() const { return snapshot_; }

    private:
        SCSim::UnitData snapshot_ = {};
    };
}


