#pragma once
#include <Engine/Game/GameObject.h>

#include <SCSimulator/Include/SCSimulatorTypes.h>

namespace engine
{
    class SCUnit;
    class SCUnitBehavior;

    class SCUnitObj :
        public GameObject
    {
        ENTITY_INFO(SCUnitObj, GameObject);

	public:
        SCUnitObj();
        virtual ~SCUnitObj() override;

		virtual void Init() override;


        void SetUnitID(uint32_t id) { unit_id_ = id; }
        uint32_t GetUnitID() const { return unit_id_; }

    private:
        SCUnit* GetSCUnit() const { return sc_unit_; }
		SCUnitBehavior* GetSCUnitBehavior() const { return sc_unit_behavior_; }

    private:
        uint32_t unit_id_ = SCSim::kInvalidUnitId;
		SCUnit* sc_unit_ = nullptr;
		SCUnitBehavior* sc_unit_behavior_ = nullptr;
    };
}


