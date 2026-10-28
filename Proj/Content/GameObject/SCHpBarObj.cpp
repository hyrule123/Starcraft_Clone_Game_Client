#include "Content/pch.h"
#include "SCHpBarObj.h"

#include <Engine/Game/Component/Transform.h>

#include <Content/HLSL/SCImage/SCHpBar.hlsli>
#include <Content/SCData/SCResourceManager.h>
#include <Content/SCData/SCImageLibrary.h>

#include <Content/Component/SCHpBarRenderer.h>

#include <Content/SCData/SCSpriteCommon.h>

namespace sc
{
	namespace
	{
		int32_t GetFilledWidth(int32_t percent, int32_t bar_width)
		{
			int32_t ret = percent * bar_width / 100;
			if (ret < 3) { ret = 3; }						//min = 3px
			else if (ret % 3 > 1) { ret += 3 - ret % 3; }	//ceil
			else if (ret % 3 == 1) { ret = ret - 1; }		//floor

			return ret;
		}


		SCHpBarInstance BuildHpBarInstance(const SCHpBarObj::ApplyHpBarArgs& args, int32_t circle_frame_height)
		{
			SCHpBarInstance data = {};

			const int32_t w = args.sprite_type_data.health_bar_size;
			data.bar_width = static_cast<uint32_t>(w - (w - 1) % 3);	// 3k + 1단위로 맞춤(구분선이 양 끝에 올 수 있게)
			if (data.bar_width < 19) { data.bar_width = 19; }	// 원작: 최소 19px

			const bool has_shield = (args.vitals.flags & SCSim::UnitVitalsFlags::HasShield) != 0;
			const bool has_skill_energy = (args.vitals.flags & SCSim::UnitVitalsFlags::HasEnergy) != 0;
			const bool has_energy = 
				has_skill_energy
				||
				(args.unit_data.status_flags & SCSim::UnitStatusFlags::Hallucination) != 0
				||
				args.unit_data.unit_type == SCSim::UnitType::ZergBroodling;


			data.bar_flags |= has_shield ? kHpBarFlagShield : 0;
			data.bar_flags |= has_energy ? kHpBarFlagEnergy : 0;

			const int32_t width = static_cast<int32_t>(data.bar_width);
			const int32_t height = static_cast<int32_t>(GetHpBarHeight(data.bar_flags));

			// 원작: sprite->position + (0, vpos + frame.size.y / 2 + 8) 를 바 중심으로, 폭·높이 절반만큼 좌상단으로
			const int32_t offset_y = args.sprite_type_data.selection_circle_vpos + circle_frame_height / 2 + 8;
			data.sc_lt_pos = int2{ args.sprite_x - width / 2, args.sprite_y + offset_y - height / 2 };
			
			// unit_hp_percent: ceil
			// max_visible_hp >= 1은 OpenBW 내부 구현이 보장 — 경계 계약은 아니므로 ASSERT 검사 진행
			ASSERT(args.vitals.max_visible_hp >= 1);
			const int32_t hp_percent = args.vitals.hp.Ceil() * 100 / std::max(args.vitals.max_visible_hp, 1);
			data.hp_fill = static_cast<uint32_t>(GetFilledWidth(hp_percent, width));
			data.hp_stage = hp_percent >= 66 ?  kHpBarStageHigh 
						  : (hp_percent >= 33 ? kHpBarStageMid : 
												kHpBarStageLow);

			// Shield: Floor + max(1, max_shield) 보정.
			if (has_shield)
			{
				const int32_t shield_percent = args.vitals.shield.Floor() * 100 / std::max(args.vitals.max_shield, 1);
				data.shield_fill = static_cast<uint32_t>(GetFilledWidth(shield_percent, width));
			}
			
			if (has_energy)
			{
				int32_t energy_percent = 0;
				// 스킬에 쓰는 실제 Energy 보유 시
				if (has_skill_energy)
				{
					energy_percent = args.vitals.energy.Floor() * 100 / std::max(args.vitals.max_energy.Floor(), 1);
				}
				// Hallucination, Broodling: Energy는 0~100%로 표시 = 남은 수명 비율 (remove_timer / default_remove_timer)
				// 환영·브루들링: 남은 수명 비율. 원작 확인 완료 — 가득 찬 상태에서 점차 감소
				// OpenBW는 *100이 빠져 항상 최소 폭(3px)으로 그려짐(버그) → 보정
				else
				{
					energy_percent = args.vitals.remove_timer * 100 / std::max(args.vitals.default_remove_timer, 1);
				}
				data.energy_fill = static_cast<uint32_t>(GetFilledWidth(energy_percent, width));
			}

			data.depth_key = args.depth_key;
			
			return data;
		}
	}

	SCHpBarObj::SCHpBarObj()
		: Super(SCHpBarObj::kClassConcreteName)
	{}
	SCHpBarObj::~SCHpBarObj()
	{}
	void SCHpBarObj::Init()
	{
		Super::Init();

		renderer_ = AddComponent<SCHpBarRenderer>();

		// Awake의 IsRenderReady 검사 전에 Mesh·Material 설정 (Material은 모든 바가 공유)
		SCResourceManager& resource_manager = SCResourceManager::GetInst();
		renderer_->SetMesh(resource_manager.GetImageMesh());
		renderer_->SetMaterial(0, resource_manager.GetHpBarMaterial());
	}
	void SCHpBarObj::ApplyHpBar(const ApplyHpBarArgs& args)
	{
		// 선택 원 GRP 프레임 0 높이 — 원작: selection_circle_vpos + frame.size.y / 2 + 8
		const int32_t circle_image_type = SCSim::kImageTypeSelectionCircleBase + args.sprite_type_data.selection_circle;
		const int32_t circle_frame_height = static_cast<int32_t>(
			SCResourceManager::GetInst().GetSCImageLibrary().GetFrameRect(circle_image_type, 0).size.y);

		const SCHpBarInstance inst = BuildHpBarInstance(args, circle_frame_height);

		// Transform은 컬링 전용 — 그리기 위치는 inst.sc_lt_pos (같은 값에서 설정, 이 함수가 유일한 기록 지점)
		const float2 pos = ScIntToWorld(inst.sc_lt_pos.x, inst.sc_lt_pos.y);
		const float3 world_pos = { pos.x, pos.y, kSpriteDefaultZ };	// 루트 오브젝트라 로컬 = 월드
		engine::Transform* transform = GetTransform();
		if (transform->GetLocalPosition() != world_pos)
		{
			transform->SetLocalPosition(world_pos);
		}

		// Renderer에 데이터 전송
		renderer_->Apply(inst);
	}
}