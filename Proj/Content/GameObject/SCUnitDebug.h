#pragma once
#include <Engine/Game/GameObject.h>

#include <SCSimulator/Include/SCSimulatorTypes.h>

namespace engine
{
    class SCUnitDebug :
        public GameObject
    {
        ENTITY_INFO(SCUnitDebug, GameObject);

	public:
        SCUnitDebug();
        virtual ~SCUnitDebug() override;

        virtual void Init() override;

		void SetUnitId(uint32 unit_id) { unit_id_ = unit_id; }
		void InvalidateUnitId() { unit_id_ = SCSim::kInvalidUnitId; }
		uint32 GetUnitId() const { return unit_id_; }

    private:
        uint32 unit_id_ = SCSim::kInvalidUnitId;
    };
}


