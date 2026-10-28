#pragma once
#include <Engine/Game/GameObject.h>

#include <Content/SCData/SCSpriteCommon.h>

#include <SCSimulator/Include/SCSimulatorTypes.h>

namespace sc
{
    class SCImageRenderer;

    class SCSelectionCircleObj 
        : public engine::GameObject
    {
        ENTITY_INFO(SCSelectionCircleObj, engine::GameObject);
	public:
		SCSelectionCircleObj();
		virtual ~SCSelectionCircleObj() override;

        virtual void Init() override;

        void ApplySelection(int32_t sprite_x, int32_t sprite_y, const SCSim::SpriteTypeData& sprite_type_data, uint32_t depth_key, SelectionColorGroup color_group);

    private:
        SCImageRenderer* renderer_ = {};
    };
}


