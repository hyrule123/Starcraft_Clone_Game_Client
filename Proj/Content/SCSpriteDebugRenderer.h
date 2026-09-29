#pragma once
#include <Engine/Game/Component/Renderer.h>

namespace engine
{
    class SCSpriteDebugRenderer :
        public Renderer
    {
        ENTITY_INFO(SCSpriteDebugRenderer, Renderer);

    public:
		SCSpriteDebugRenderer();
		virtual ~SCSpriteDebugRenderer() override;

		virtual void Init() override;
        virtual void Awake() override;
        virtual void LateUpdate() override;

		virtual void WritePerObjectData(DataBlock data_block) override;

    private:
        uint16 bound_image_type_ = {};
    };
}


