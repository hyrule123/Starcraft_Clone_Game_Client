#pragma once
#include <Engine/Game/Scene.h>

#include <array>
#include <span>

#include <Engine/ThirdParty/ankerl/unordered_dense.h>

#include <Content/SCData/SCSelectionSystem.h>
#include <Content/SCData/SCCommandSystem.h>

#include <SCSimulator/Include/SCSimulatorTypes.h>

namespace engine
{
	class ConstantBuffer;
    class Texture2DArray;
}

namespace sc
{
    class SCUnitObj;
    class SCImageLibrary;
    
    class SCSpriteObj;
    class SCImageObj;
    class SCSelectionCircleObj;
    class SCHpBarObj;

    class SCMapRenderer;

    class CameraMoveScript;

    struct UnitEntry {
        engine::weak_handle_ptr<SCUnitObj> object = {};
        uint32_t serial = 0;
    };

    struct SpriteSlot
    {
        engine::weak_handle_ptr<SCSpriteObj> object;	// 슬롯이 처음 쓰일 때 생성, 이후 계속 유지
        uint32_t serial = 0;				// 마지막으로 본 싱크 번호
        uint32_t selected_serial = 0;	// == sprite_sync_serial_ 이면 선택됨
        uint32_t selected_slot = 0;		// selected_serial이 유효할 때만 의미 있음 (Pool 인덱스)
        uint32_t undetected_serial = 0;

        // serial == sprite_sync_serial_일 때만 유효
        uint32_t rank = 0;		    // 이번 싱크 그리기 순위, 클수록 카메라와 가까움(앞)
        uint32_t snapshot_idx = 0;	// 이번 싱크 SpriteSnapshot::sprites 내 위치
    };

    struct ImageSlot
    {
        engine::weak_handle_ptr<SCImageObj> object;
        uint32_t serial = 0;
        uint32_t sprite_index = SCSim::kInvalidSpriteIndex;	// 현재 부모 스프라이트 슬롯 (재부모화 판정)
    };

    struct SelectionSlot
    {
		engine::weak_handle_ptr<SCSelectionCircleObj> circle_obj;
		engine::weak_handle_ptr<SCHpBarObj> hp_bar_obj;
        uint32_t unit_id = SCSim::kInvalidUnitId;				    // 마킹 단계에서 기록 (체력 바 Vitals 조회용)
        uint32_t circle_drawn_serial = 0;							// 실제로 원을 그린 싱크. 아니면 정리 루프에서 SetActive(false)
		uint32_t hp_bar_drawn_serial = 0;						    // 실제로 바를 그린 싱크. 아니면 정리 루프에서 SetActive(false)
    };

    class MainGameScene :
        public engine::Scene
    {
        ENTITY_INFO(MainGameScene, engine::Scene);
    public:
        MainGameScene();
        virtual ~MainGameScene() override;

        virtual void Init() override;
        virtual void OnActivate() override;

        virtual void OnFrameStart() override;

        virtual void OnShutdown() override;

		bool LoadMap(const std::filesystem::path& map_path, bool create_map_units);

        SCSim::Context* GetSCSimContext() const { return sc_sim_ctx_; }
    
        // 시스템용 조회 API
        // unit_objects_ → SCUnit 캐시, 없으면 nullptr
        const SCSim::UnitData* GetUnitData(uint32_t unit_id) const;	

        // 이번 싱크에 본 슬롯의 SCSpriteObj 캐시, 아니면 nullptr
        const SCSim::SpriteData* GetSpriteData(uint32_t sprite_index) const;

        // 시야(visibility_flags) + 미탐지 적 클로킹 제외
        bool IsUnitVisibleToLocal(const SCSim::UnitData& unit) const;	

        // 클릭 판정, 없으면 kInvalidUnitId
        uint32_t PickUnitAt(int2 sc_pos) const;		

        SCSim::Player GetLocalPlayer() const { return local_player_; }

		engine::weak_handle_ptr<engine::Camera> GetMainCamera() const { return main_camera_; }

    private:
        void DefaultEngineSetting();
        void SyncUnits();
        void SyncSprites();
        void SyncMegatileStates();

        void UpdatePlayerInput();	// FrameStart 끝(싱크 이후)에서 호출

    private:
        SCSim::Context* sc_sim_ctx_ = {};
        float acc_delta_time_ = {}; // for SCSimContext update

        bool is_map_loaded_ = {};

        SCSelectionSystem selection_system_;
        SCCommandSystem command_system_;

#pragma region SNAPSHOTS
        // ===== 초기화 시 1회 =====
        // 소유자: SCSim::Context, 이 클래스와 수명을 같이 하므로 에러 위험 없음
		SCSim::GameInfo game_info_ = {};
        std::span<const SCSim::SpriteTypeData> sprite_types_ = {};
        // =======================
        
        // ===== 매 프레임 =====
        std::span<const SCSim::UnitData> sc_units_ = {};
		std::span<const SCSim::SpriteData> sprite_snapshots_ = {};
		std::span<const SCSim::ImageData> image_snapshots_ = {};
        // ====================
#pragma endregion SNAPSHOTS

        // Unit ID - UnitEntry pair 
        uint32_t sync_serial_ = 0;
        ankerl::unordered_dense::map<uint32_t, UnitEntry> unit_objects_ = {};

        // 스프라이트 이미지
        std::array<SpriteSlot, SCSim::kMaxSprites> sprite_slots_ = {};
        std::array<ImageSlot, SCSim::kMaxImages> image_slots_ = {};

        std::array<SCSim::PlayerData, static_cast<size_t>(SCSim::Player::kCount)> player_data_ = {};
        int32_t player_count_ = 0;

        // SyncSprites 정렬용: (정렬 키 << 32) | 스냅샷 위치
        std::array<uint64_t, SCSim::kMaxSprites> sprite_sort_buffer_ = {};

        uint32_t sprite_sync_serial_ = 0;

        // Selection
		std::array<SelectionSlot, SCSim::kMaxSelection> selection_pool_ = {};

        std::shared_ptr<engine::ConstantBuffer> sc_common_info_buffer_ = {};

        engine::weak_handle_ptr<SCMapRenderer> map_renderer_ = {};

        SCSim::Player local_player_ = {};

        // Camera
		engine::weak_handle_ptr<engine::Camera> main_camera_ = {};
		engine::weak_handle_ptr<CameraMoveScript> camera_move_script_ = {};
    };
}