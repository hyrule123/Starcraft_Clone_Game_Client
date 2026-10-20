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

		void SetWPETargets(
			std::shared_ptr<engine::RenderTarget> rt, std::shared_ptr<engine::DepthStencilTarget> dst, 
			std::shared_ptr<engine::RenderTarget> mirror);

	protected:
		virtual void BindTargets(const engine::RenderContext& ctx) override;
		virtual void UnbindTargets(const engine::RenderContext& ctx) override;

	private:
		std::shared_ptr<engine::RenderTarget> wpe_render_target_ = {};
		std::shared_ptr<engine::DepthStencilTarget> wpe_depth_target_ = {};

		// Distortion 구현을 위한 Mirror Target.
		// Distortion 구현은 몇 칸 옆의 픽셀 데이터를 가져오는데, ROV는 자신의 픽셀 이외의 픽셀은 읽기/쓰기 순서를 보장하지 않음.
		// -> 몇 칸 옆 픽셀이 다른 Distortion 유닛이 수정한 픽셀일 수도 있고, 아닐 수도 있음 -> 이를 해결하기 위해 '원본'을 두고 거기서 가져와야 함
		// 이를 해결하기 위해 Distortion 유닛은 2회 렌더링한다.
		// 첫 번째 렌더링 시 RenderTarget에 렌더링 -> 두 번째 렌더링 시 렌더링될 픽셀을 MirrorTarget에 복사
		std::shared_ptr<engine::RenderTarget> wpe_mirror_target_ = {};
    };
}


