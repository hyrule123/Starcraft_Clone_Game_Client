#pragma once
#include <Engine/Game/GameObject.h>

#include <SCSimulator/Include/SCSimulatorTypes.h>

namespace sc
{
    class SCUnitDebug :
        public engine::GameObject
    {
        ENTITY_INFO(SCUnitDebug, ::engine::GameObject);

	public:
        SCUnitDebug();
        virtual ~SCUnitDebug() override;

        virtual void Init() override;

		void SetUnitId(uint32_t unit_id) { unit_id_ = unit_id; }
		void InvalidateUnitId() { unit_id_ = SCSim::kInvalidUnitId; }
		uint32_t GetUnitId() const { return unit_id_; }

    private:
        uint32_t unit_id_ = SCSim::kInvalidUnitId;
    };
}


