#include "Content/pch.h"
#include "SCEffectPass.h"

#include <Engine/Resource/GPU/Buffer/RenderTarget.h>
#include <Engine/Resource/GPU/Buffer/DepthStencilTarget.h>



namespace sc
{
	SCEffectPass::SCEffectPass(engine::render_pass::ID id, SortPolicy sort_policy)
		: Super(SCEffectPass::kClassConcreteName, id, sort_policy)
	{
	}

	SCEffectPass::~SCEffectPass()
	{}
	void SCEffectPass::SetWPETargets(std::shared_ptr<engine::RenderTarget> rt, std::shared_ptr<engine::DepthStencilTarget> dst)
	{
		CHECK(rt && dst);
		CHECK(rt->GetRawUAV());

		CHECK(rt->GetWidth() == dst->GetWidth() && rt->GetHeight() == dst->GetHeight());

		wpe_render_target_ = rt;
		wpe_depth_target_ = dst;
	}
	void SCEffectPass::BindTargets(const engine::RenderContext& ctx)
	{
		ASSERT(wpe_render_target_ && wpe_depth_target_);

		ID3D11UnorderedAccessView* wpe_idx_uav = wpe_render_target_->GetRawUAV();
		ID3D11DepthStencilView* wpe_depth_dsv = wpe_depth_target_->GetRawDepthStencilView();
		constexpr UINT initial_counts = 0;
		
		// ROV만 바인딩함
		ctx.dc->OMSetRenderTargetsAndUnorderedAccessViews(0, nullptr, wpe_depth_dsv, 0, 1, &wpe_idx_uav, &initial_counts);

		D3D11_VIEWPORT viewport = {};
		viewport.Width = static_cast<FLOAT>(wpe_render_target_->GetWidth());
		viewport.Height = static_cast<FLOAT>(wpe_render_target_->GetHeight());
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;

		ctx.dc->RSSetViewports(1, &viewport);
	}
	void SCEffectPass::UnbindTargets(const engine::RenderContext & ctx)
	{
		constexpr ID3D11UnorderedAccessView* null_uav = nullptr;
		constexpr ID3D11DepthStencilView* null_dsv = nullptr;

		// ROV만 해제함
		ctx.dc->OMSetRenderTargetsAndUnorderedAccessViews(0, nullptr, null_dsv, 0, 1, &null_uav, nullptr);
	}
}
