#pragma once
#include <Engine/Game/Component/Renderer.h>

namespace sc
{
    class SCSpriteDebugRenderer :
        public engine::Renderer
    {
        ENTITY_INFO(SCSpriteDebugRenderer, engine::Renderer);

    public:
        SCSpriteDebugRenderer();
        virtual ~SCSpriteDebugRenderer() override;

        virtual void Init() override;
        virtual void Awake() override;
        virtual void LateUpdate() override;

        virtual void WriteInstanceData(engine::DataBlock data_block) override;

    private:
        uint16 bound_image_type_ = {};
    };
}


