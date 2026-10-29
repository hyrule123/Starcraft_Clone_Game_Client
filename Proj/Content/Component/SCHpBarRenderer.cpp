#include "Content/pch.h"
#include "SCHpBarRenderer.h"

#include <Content/SCData/Constants.h>
#include <Content/SCData/Image/SCSpriteCommon.h>

#include <Content/HLSL/SCImage/SCHpBar.hlsli>

namespace sc
{
	SCHpBarRenderer::SCHpBarRenderer()
		: Super(SCHpBarRenderer::kClassConcreteName)
	{}

	SCHpBarRenderer::~SCHpBarRenderer()
	{}
	void SCHpBarRenderer::Awake()
	{
		Super::Awake();

		CHECK(IsRenderReady() == true);
	}
	void SCHpBarRenderer::Apply(const SCHpBarInstance& inst)
	{
		// 로컬 bounds: Transform 원점 = 바 좌상단, 월드 y-up이므로 바는 -y 방향. 1픽셀 여유
		const float w = static_cast<float>(inst.bar_width);
		const float h = static_cast<float>(GetHpBarHeight(inst.bar_flags));
		SetLocalBoundsOverride(engine::AABB3D{ .min = float3(-1.0f, -h - 1.0f, 0.0f), .max = float3(w + 1.0f, 1.0f, 0.0f) });

		SetCustomSortKey(inst.depth_key);

		inst_cache_ = inst;
	}
	void SCHpBarRenderer::WriteInstanceData(const InstanceWriteArgs& args)
	{
		switch (args.pass)
		{
		case kSCOpaquePassID:
		{
			args.block.Write(inst_cache_);
		}
		break;
		default:
			CHECK_F(false, "처리되지 않은 Pass ID: {}", static_cast<uint32_t>(args.pass));
		}
	}
}


