#pragma once
#include <Engine/Game/Component/Renderer.h>

#include <SCSimulator/Include/SCSimulatorTypes.h>
#include <Content/SCData/SCSpriteCommon.h>

namespace engine
{
	class Texture2DArray;
}

namespace sc
{
    struct ImageDef;

    class SCImageRenderer :
        public engine::Renderer
    {
		ENTITY_INFO(SCImageRenderer, engine::Renderer);

    public:
        SCImageRenderer();
		virtual ~SCImageRenderer() override;
        
        virtual void Awake() override;

		void ApplySnapshot(const SCSim::ImageData& img_data, const SCImageParam& img_param);

		virtual void WriteInstanceData(const InstanceWriteArgs& args) override;

    private:
        SCSim::ImageData img_data_cache_ = {};
        SCImageParam img_param_cache_ = {};
        const ImageDef* img_def_cache_ = {};
    };
}


