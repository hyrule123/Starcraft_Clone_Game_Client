#pragma once
#include <Engine/Game/Component/Renderer.h>

#include <Engine/Core/CoreMinimal.h>

#include <Content/Shader/SCSprite.hlsli>

namespace engine
{
    class SCSpriteRenderer :
        public Renderer
    {
        CLASS_INFO(SCSpriteRenderer, Renderer)
    public:
        SCSpriteRenderer();
        virtual ~SCSpriteRenderer() override;

        virtual void Init() override;
        virtual void Awake() override;
        virtual void LateUpdate() override;

        virtual void WritePerObjData(void* ptr) final;

        void SetFrameIndex(uint32 frame_index) { per_obj_data_.sprite_frame_idx = frame_index; }

    private:
        SCSpriteInstanceData per_obj_data_ = {};
        RenderPassOrder render_pass_mode_ = RenderPassOrder::kForwardOpaque;
    };
}
