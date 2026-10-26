#pragma once
#include <Engine/Game/Component/Renderer.h>

namespace SCSim
{
	struct MegatileState;
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

		// SCSim_GetMegatileStates 결과를 그대로 업로드 (매 프레임). 맵 크기가 SCMap과 다르면 false
		[[nodiscard]] bool ApplyMegatileSnapshot(const SCSim::MegatileState* megatiles, uint32_t map_width_megatile, uint32_t map_height_megatile);

	private:
		std::unique_ptr<SCMap> sc_map_ = {};

		std::shared_ptr<engine::TypedBuffer> megatile_states_ = {};

		bool needs_scale_matching_ = {};
    };
}


