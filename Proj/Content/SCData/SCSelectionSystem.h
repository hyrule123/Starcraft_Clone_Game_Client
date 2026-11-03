#pragma once
#include <Content/SCData/SCCommon.h>

// TODO: 선택된 유닛이 시야에서 사라질 시 제거하는 코드 추가 필요
namespace sc
{
	class MainGameScene;

	class SCSelectionSystem
	{
	public:
		explicit SCSelectionSystem(MainGameScene* owner_scene);

		void Update(const PlayerInputFrame& player_input);
		bool IsDragging() const { return is_dragging_; }

	private:
		void SelectByClick(uint32_t unit_id);
		void SelectByRect(int2 sc_min, int2 sc_max);	// FindUnitsInRect → 정제 → SCSim_Select
		void DrawDragBox(int2 drag_to) const;

		MainGameScene* owner_scene_ = {};
		bool is_dragging_ = {};
		int2 drag_from_ = {};		// 드래그 시작점 (SC 좌표, 드래그 중 카메라 잠금 전제)
	};
}