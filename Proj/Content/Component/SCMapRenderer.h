#pragma once
#include <Engine/Game/Component/Renderer.h>

namespace SCSim
{
	struct TileData;
}

namespace engine
{
	class TypedBuffer;
}

namespace sc
{
	struct SCMap;
	class SCResourceManager;
	class Texture2D;
	class ConstantBuffer;
    class SCMapRenderer :
        public engine::Renderer
    {
		ENTITY_INFO(SCMapRenderer, engine::Renderer);

    public:
		SCMapRenderer();
		virtual ~SCMapRenderer() override;

		virtual void Init() override;
		//virtual void Awake() override;

		virtual void LateUpdate() override;

		virtual void OnDestroy() override;

		virtual void WriteInstanceData(const InstanceWriteArgs& args) override;

		void SetSCMap(std::unique_ptr<SCMap> sc_map);

		// SCSim_GetTiles 결과를 그대로 업로드 (매 프레임). 맵 크기가 SCMap과 다르면 false
		[[nodiscard]] bool ApplyTileSnapshot(const SCSim::TileData* tiles, uint32_t width, uint32_t height);

	private:
		std::unique_ptr<SCMap> sc_map_ = {};

		std::shared_ptr<engine::TypedBuffer> tile_states_ = {};

		bool needs_scale_matching_ = {};
    };
}


