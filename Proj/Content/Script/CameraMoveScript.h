#pragma once
#include <Engine/Game/Component/Script.h>

namespace sc
{
    class CameraMoveScript :
        public engine::Script
    {
        ENTITY_INFO(CameraMoveScript, engine::Script);

	public:
        CameraMoveScript();
        virtual ~CameraMoveScript() override;
        
        virtual void Update() override;

    private:
        float speed_;
        const float pos_update_interval_;
        float pos_update_timer_ = 0.0f;
    };
}


