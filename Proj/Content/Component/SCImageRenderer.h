#pragma once
#include <Engine/Game/Component/Renderer.h>

#include <SCSimulator/Include/SCSimulatorTypes.h>

namespace engine
{
	class Texture2DArray;
}

namespace sc
{

    class SCImageRenderer :
        public engine::Renderer
    {
		ENTITY_INFO(SCImageRenderer, engine::Renderer);

    public:
        SCImageRenderer();
		virtual ~SCImageRenderer() override;
        
        virtual void Awake() override;

		void ApplySnapshot(const SCSim::ImageData& img_data, uint8_t player_color);

		virtual void WritePerObjectData(engine::DataBlock data_block) override;

    private:
        SCSim::ImageData img_data_cache_ = {};
        uint8_t player_color_ = {};
    };
}


