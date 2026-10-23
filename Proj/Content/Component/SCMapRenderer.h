#pragma once
#include <Engine/Game/Component/Renderer.h>

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

	private:
		std::unique_ptr<SCMap> sc_map_ = nullptr;

		bool needs_scale_matching_ = false;
    };
}


