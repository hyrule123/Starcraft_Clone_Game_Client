#include "Content/pch.h"
#include "SCEffectPass.h"

#include <Engine/Resource/GPU/Buffer/RenderTarget.h>
#include <Engine/Resource/GPU/Buffer/DepthStencilTarget.h>

#include <Content/HLSL/SCImage/SCImageCommon.hlsli>

namespace sc
{
	constexpr UINT kEffectPassUavCount = 2;	// ROV에 바인딩할 UAV 개수.

	// 연속되는지 확인
	static_assert(REG_U_SC_WPE_TO_ROV_SLOT.Get() + 1 == REG_U_SC_WPE_MIRROR_SLOT.Get());

	SCEffectPass::SCEffectPass(engine::render_pass::ID id, SortPolicy sort_policy)
		: Super(SCEffectPass::kClassConcreteName, id, sort_policy)
	{
		ASSERT(sort_policy == SortPolicy::Custom);
	}

	SCEffectPass::~SCEffectPass()
	{}

	void SCEffectPass::SetWPETargets(
		std::shared_ptr<engine::RenderTarget> rt, std::shared_ptr<engine::DepthStencilTarget> dst,
		std::shared_ptr<engine::RenderTarget> mirror)
	{
		CHECK(rt && dst && mirror);
		CHECK(rt->GetRawUAV() && mirror->GetRawUAV());

		CHECK(rt->GetWidth() == dst->GetWidth() && rt->GetHeight() == dst->GetHeight());
		CHECK(rt->GetWidth() == mirror->GetWidth() && rt->GetHeight() == mirror->GetHeight());
		CHECK(rt->GetFormat() != DXGI_FORMAT_UNKNOWN && rt->GetFormat() == mirror->GetFormat());	// getter가 있다면

		wpe_render_target_ = rt;
		wpe_depth_target_ = dst;
		wpe_mirror_target_ = mirror;
	}

	void SCEffectPass::BindTargets(const engine::RenderContext& ctx)
	{
		ASSERT(wpe_render_target_ && wpe_depth_target_ && wpe_mirror_target_);

		wpe_mirror_target_->CopyFrom(ctx.dc, *wpe_render_target_);

		ID3D11UnorderedAccessView* uavs[] = { wpe_render_target_->GetRawUAV(), wpe_mirror_target_->GetRawUAV() };
		static_assert(std::size(uavs) == kEffectPassUavCount);

		ID3D11DepthStencilView* wpe_depth_dsv = wpe_depth_target_->GetRawDepthStencilView();

		// ROV만 바인딩함
		ctx.dc->OMSetRenderTargetsAndUnorderedAccessViews(0, nullptr, wpe_depth_dsv, REG_U_SC_WPE_TO_ROV_SLOT.Get(), kEffectPassUavCount, uavs, nullptr);

		D3D11_VIEWPORT viewport = {};
		viewport.Width = static_cast<FLOAT>(wpe_render_target_->GetWidth());
		viewport.Height = static_cast<FLOAT>(wpe_render_target_->GetHeight());
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;

		ctx.dc->RSSetViewports(1, &viewport);
	}
	void SCEffectPass::UnbindTargets(const engine::RenderContext& ctx)
	{
		constexpr ID3D11UnorderedAccessView* null_uav[kEffectPassUavCount] = {};
		constexpr ID3D11DepthStencilView* null_dsv = nullptr;

		// ROV A, B 해제
		ctx.dc->OMSetRenderTargetsAndUnorderedAccessViews(0, nullptr, null_dsv, REG_U_SC_WPE_TO_ROV_SLOT.Get(), kEffectPassUavCount, null_uav, nullptr);
	}
}
