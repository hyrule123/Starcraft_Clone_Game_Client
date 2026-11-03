#include "Content/pch.h"
#include "SCSelectionSystem.h"

#include <Engine/Manager/RenderManager.h>

#include <Content/Scene/MainGameScene.h>

#include <SCSimulator/Include/SCSimulatorAPI.h>

namespace sc
{
	SCSelectionSystem::SCSelectionSystem(MainGameScene* owner_scene)
		: owner_scene_(owner_scene)
	{}
	void SCSelectionSystem::Update(const PlayerInputFrame& player_input)
	{
		// 원작: 폭 또는 높이가 이 값 이하면 드래그가 아니라 클릭 (OpenBW end_drag_select)
		constexpr int32_t kClickThreshold = 4;

		// down을 먼저 처리해야 같은 프레임에 down/up이 함께 와도 클릭으로 처리됨
		if (player_input.left_down)
		{
			is_dragging_ = true;
			drag_from_ = player_input.mouse_sc;
		}

		if (!is_dragging_)
		{
			return;
		}

		DrawDragBox(player_input.mouse_sc);

		if (player_input.left_up)
		{
			is_dragging_ = false;

			int2 from = drag_from_;
			int2 to = player_input.mouse_sc;
			if (from.x > to.x) { std::swap(from.x, to.x); }
			if (from.y > to.y) { std::swap(from.y, to.y); }

			if (to.x - from.x <= kClickThreshold || to.y - from.y <= kClickThreshold)
			{
				// 정규화한 from(좌상단)으로 판정 — OpenBW와 동일
				SelectByClick(owner_scene_->PickUnitAt(from));
			}
			else
			{
				SelectByRect(from, to);
			}
		}
	}
	void SCSelectionSystem::SelectByClick(uint32_t unit_id)
	{
		if (unit_id == SCSim::kInvalidUnitId) { return; }
		int32_t count = SCSim_Select(owner_scene_->GetSCSimContext(), owner_scene_->GetLocalPlayer(), &unit_id, 1);
		CHECK_F(0 <= count, "Select Failed: {}", SCSim_GetLastError());
	}

	void SCSelectionSystem::SelectByRect(int2 sc_min, int2 sc_max)
	{
		SCSim::Context* sc_ctx = owner_scene_->GetSCSimContext();
		const SCSim::Player local_player = owner_scene_->GetLocalPlayer();
		const int32_t local_id = static_cast<int32_t>(local_player);
		constexpr int32_t kNeutralId = static_cast<int32_t>(SCSim::Player::Neutral);

		const uint32_t* found_units = nullptr;
		const int32_t count = SCSim_FindUnitsInRect(sc_ctx, sc_min.x, sc_min.y, sc_max.x, sc_max.y, &found_units);
		CHECK_F(0 <= count, "SCSim_FindUnitsInRect failed: {}", SCSim_GetLastError());

		// 1차: 거르면서 분류 (finder 순서 유지). 결과 버퍼는 공유되므로 이 루프에서 바로 소비
		std::vector<uint32_t> own_units;
		uint32_t first_other = SCSim::kInvalidUnitId;		// 내 것이 아닌 비중립(적·동맹)
		uint32_t first_neutral = SCSim::kInvalidUnitId;

		for (const uint32_t unit_id : std::span(found_units, static_cast<size_t>(count)))
		{
			const SCSim::UnitData* unit_data = owner_scene_->GetUnitData(unit_id);
			if (unit_data == nullptr) { continue; }
			if (!IsSelectableUnitType(unit_data->unit_type)) { continue; }
			if (!owner_scene_->IsUnitVisibleToLocal(*unit_data)) { continue; }

			if (unit_data->owner == local_id)
			{
				own_units.push_back(unit_id);
			}
			else if (unit_data->owner == kNeutralId)
			{
				if (first_neutral == SCSim::kInvalidUnitId) { first_neutral = unit_id; }
			}
			else
			{
				if (first_other == SCSim::kInvalidUnitId) { first_other = unit_id; }
			}
		}

		std::vector<uint32_t> selection;

		if (!own_units.empty())
		{
			// 출력 버퍼는 호출자 소유 (count칸)
			std::vector<uint32_t> select_flags(own_units.size(), 0);
			const bool result = SCSim_GetSelectInfo(sc_ctx, own_units.data(), static_cast<int32_t>(own_units.size()), select_flags.data());
			CHECK_F(result, "SCSim_GetSelectInfo failed: {}", SCSim_GetLastError());

			// 2차: 내 유닛 중 다중 선택 가능한 것만 (첫 원소 포함), 최대 12개
			for (size_t i = 0; i < own_units.size(); ++i)
			{
				if ((select_flags[i] & SCSim::SelectInfoFlags::MultiSelectable) == 0) { continue; }

				selection.push_back(own_units[i]);
				if (selection.size() >= static_cast<size_t>(SCSim::kMaxSelection)) { break; }
			}

			// 다중 선택 가능한 유닛이 없음(건물만) → 첫 번째 하나
			if (selection.empty())
			{
				selection.push_back(own_units[0]);
			}
		}
		else if (first_other != SCSim::kInvalidUnitId)
		{
			selection.push_back(first_other);		// 적·동맹 단독 (비중립이 있으면 중립 제외)
		}
		else if (first_neutral != SCSim::kInvalidUnitId)
		{
			selection.push_back(first_neutral);
		}

		// 빈 영역 드래그: 기존의 선택 상태를 건드리지 않음
		if (selection.empty()) { return; }

		const int32_t selected = SCSim_Select(sc_ctx, local_player, selection.data(), static_cast<int32_t>(selection.size()));
		CHECK_F(0 <= selected, "SCSim_Select failed: {}", SCSim_GetLastError());	// 0개 선택은 정상
	}

	void SCSelectionSystem::DrawDragBox(int2 drag_to) const
	{
		DebugDrawPerInstanceData dbg_rect = {};

		const float2 pos = ScToWorld(static_cast<float>(drag_from_.x + drag_to.x) * 0.5f, static_cast<float>(drag_from_.y + drag_to.y) * 0.5f);

		const float3 scale = { static_cast<float>(std::abs(drag_from_.x - drag_to.x)), static_cast<float>(std::abs(drag_from_.y - drag_to.y)), 1.0f };

		dbg_rect.world_mat = matrix::CreateScale(scale) * matrix::CreateTranslation(pos.x, pos.y, 0.0f);
		dbg_rect.color = float3{ 0.0f, 1.0f, 0.0f };

		engine::RenderManager::GetInst().PushDebugRect(dbg_rect);
	}
}