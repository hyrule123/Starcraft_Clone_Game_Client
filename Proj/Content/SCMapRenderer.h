#pragma once
#include <Engine/Game/Component/Renderer.h>

namespace engine
{
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

		virtual void WritePerObjData(void* ptr) override;

		void SetMapLoader(u_ptr<SCMapLoader> map_loader) { map_loader_ = std::move(map_loader); }

	private:
		s_ptr<ConstantBuffer> per_obj_cb_ = {};
		u_ptr<SCMapLoader> map_loader_ = {};
    };
}


