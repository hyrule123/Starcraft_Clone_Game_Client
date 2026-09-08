#pragma once
#include <Engine/Game/Component/Renderer.h>

namespace engine
{
	struct SCMap;
	class SCMapLoader;
	class Texture2D;
	class ConstantBuffer;
    class SCMapRenderer :
        public Renderer
    {
		ENTITY_INFO(SCMapRenderer, Renderer)
    public:
		SCMapRenderer();
		virtual ~SCMapRenderer() override;

		virtual void Init() override;
		//virtual void Awake() override;

		virtual void LateUpdate() override;

		virtual void WritePerObjectData(DataBlock data_block) override;

		void SetSCMap(u_ptr<SCMap> sc_map);

	private:
		u_ptr<SCMap> sc_map_ = nullptr;
		s_ptr<ConstantBuffer> map_info_cb_ = nullptr;

		bool needs_scale_matching_ = false;
    };
}


