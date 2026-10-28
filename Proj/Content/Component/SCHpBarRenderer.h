#pragma once
#include <Engine/Game/Component/Renderer.h>

#include <Content/HLSL/SCImage/SCHpBar.hlsli>

namespace sc
{
    class SCHpBarRenderer
        : public engine::Renderer
    {
		ENTITY_INFO(SCHpBarRenderer, engine::Renderer);

    public:
        SCHpBarRenderer();
		virtual ~SCHpBarRenderer() override;

        virtual void Awake() override;

        void Apply(const SCHpBarInstance& inst);

        virtual void WriteInstanceData(const InstanceWriteArgs& args) override;

    private:
        SCHpBarInstance inst_cache_ = {};
    };
}


