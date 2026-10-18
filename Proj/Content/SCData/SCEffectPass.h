#pragma once
#include <Engine/Render/RenderPass/MeshRenderPass.h>

namespace engine
{
	class RenderTarget;
	class DepthStencilTarget;
}

namespace sc
{
    class SCEffectPass :
        public engine::MeshRenderPass
    {
		ENTITY_INFO(SCEffectPass, engine::MeshRenderPass);

	public:
		SCEffectPass(engine::render_pass::ID id, SortPolicy sort_policy);
		virtual ~SCEffectPass() override;

		void SetWPETargets(std::shared_ptr<engine::RenderTarget> rt, std::shared_ptr<engine::DepthStencilTarget> dst);

	protected:
		virtual void BindTargets(const engine::RenderContext& ctx) override;
		virtual void UnbindTargets(const engine::RenderContext& ctx) override;

	private:
		std::shared_ptr<engine::RenderTarget> wpe_render_target_ = {};
		std::shared_ptr<engine::DepthStencilTarget> wpe_depth_target_ = {};
    };
}


