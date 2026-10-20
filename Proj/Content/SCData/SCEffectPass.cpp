#include "Content/pch.h"
#include "SCEffectPass.h"

#include <Engine/Resource/GPU/Buffer/RenderTarget.h>
#include <Engine/Resource/GPU/Buffer/DepthStencilTarget.h>

#include <Content/HLSL/SCImage/SCImageCommon.hlsli>

namespace sc
{
	SCEffectPass::SCEffectPass(engine::render_pass::ID id, SortPolicy sort_policy)
		: Super(SCEffectPass::kClassConcreteName, id, sort_policy)
	{
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
		constexpr UINT num_uav = static_cast<UINT>(sizeof(uavs) / sizeof(uavs[0]));

		ID3D11DepthStencilView* wpe_depth_dsv = wpe_depth_target_->GetRawDepthStencilView();
		
		// 연속되는지 확인
		static_assert(REG_U_SC_WPE_TO_ROV_SLOT.Get() + 1 == REG_U_SC_WPE_MIRROR_SLOT.Get());

		// ROV만 바인딩함
		ctx.dc->OMSetRenderTargetsAndUnorderedAccessViews(0, nullptr, wpe_depth_dsv, REG_U_SC_WPE_TO_ROV_SLOT.Get(), num_uav, uavs, nullptr);

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
		ctx.dc->OMSetRenderTargetsAndUnorderedAccessViews(0, nullptr, null_dsv, REG_U_SC_WPE_TO_ROV_SLOT.Get(), 1, &null_uav, nullptr);
	}
}
