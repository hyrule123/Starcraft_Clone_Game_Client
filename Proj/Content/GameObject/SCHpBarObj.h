#pragma once
#include <Engine/Game/GameObject.h>

#include <SCSimulator/Include/SCSimulatorTypes.h>

namespace sc
{
	class SCHpBarRenderer;

    class SCHpBarObj
        : public engine::GameObject
    {
		ENTITY_INFO(SCHpBarObj, engine::GameObject);
    public:
		SCHpBarObj();
		virtual ~SCHpBarObj() override;

		virtual void Init() override;

		// ApplyHpBar 입력. 호출 지점에서 만들어 바로 소비 — 보관 금지 (참조 멤버가 스냅샷 버퍼를 가리킴)
		struct ApplyHpBarArgs
		{
			int32_t sprite_x;
			int32_t sprite_y;
			uint32_t depth_key;
			const SCSim::SpriteTypeData& sprite_type_data;
			const SCSim::UnitData& unit_data;
			const SCSim::UnitVitals& vitals;
		};
		
		// 원작 규칙(OpenBW draw_health_bars)으로 바 인스턴스를 구성. 위치·폭·채움·색 단계 모두 내부에서 계산
		// 무적 유닛은 호출하지 않음 (판정은 호출 측 — 원작도 바 자체를 생략)
		void ApplyHpBar(const ApplyHpBarArgs& args);

    private:
		SCHpBarRenderer* renderer_ = {};
    };
}


