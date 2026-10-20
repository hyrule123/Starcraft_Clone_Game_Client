#pragma once
#include <Engine/Game/Scene.h>

#include <Engine/ThirdParty/ankerl/unordered_dense.h>

#include <SCSimulator/Include/SCSimulatorAPI.h>

#include <array>

namespace engine
{
	class ConstantBuffer;
}

namespace sc
{
    class SCUnitObj;
    class SCImageLibrary;
    
    class SCSpriteObj;
    class SCImageObj;

    struct UnitEntry {
        SCUnitObj* object = {};
        uint32_t serial = 0;
    };

    struct SpriteSlot
    {
        engine::weak_handle_ptr<SCSpriteObj> object;	// 슬롯이 처음 쓰일 때 생성, 이후 계속 유지
        uint32_t serial = 0;		// 마지막으로 본 싱크 번호
    };

    struct ImageSlot
    {
        engine::weak_handle_ptr<SCImageObj> object;
        uint32_t serial = 0;
        uint32_t sprite_index = SCSim::kInvalidSpriteIndex;	// 현재 부모 스프라이트 슬롯 (재부모화 판정)
    };

    class MainGameScene :
        public engine::Scene
    {
        ENTITY_INFO(MainGameScene, engine::Scene);
    public:
        MainGameScene();
        virtual ~MainGameScene() override;

        virtual void Init() override;
        virtual void FrameStart() override;

		SCSim::Context* GetSCSimContext() const { return sc_sim_ctx_; }

    private:
        void PrepareMapLoader();
		void DefaultEngineSetting();
        void SyncUnits();
        void SyncSprites();

    private:
        SCSim::Context* sc_sim_ctx_ = nullptr;
		float acc_delta_time_ = 0.0f; // for SCSimContext update

		const SCSim::UnitData* sc_units_ = {};
		int32_t sc_units_count_ = 0;

        // Unit ID - UnitEntry pair
        ankerl::unordered_dense::map<uint32_t, UnitEntry> unit_objects_ = {};
		uint32_t sync_serial_ = 0;

        // 스프라이트 이미지

        std::array<SpriteSlot, SCSim::kMaxSprites> sprite_slots_ = {};
        std::array<ImageSlot, SCSim::kMaxImages> image_slots_ = {};
		std::array<SCSim::PlayerData, static_cast<size_t>(SCSim::Player::kCount)> player_data_ = {};
        int32_t player_count_ = 0;

        // SyncSprites 정렬용: (정렬 키 << 32) | 스냅샷 위치
        std::array<uint64_t, SCSim::kMaxSprites> sprite_sort_buffer_ = {};

        uint32_t sprite_sync_serial_ = 0;

        std::shared_ptr<engine::ConstantBuffer> sc_common_info_buffer_ = {};
        
    };
}