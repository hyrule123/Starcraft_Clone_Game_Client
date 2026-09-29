#pragma once
#include <Engine/Game/Scene.h>

#include <Engine/ThirdParty/ankerl/unordered_dense.h>

#include <Content/SCData/MPQ/MPQFileSystem.h>

#include <SCSimulator/Include/SCSimulatorAPI.h>

#include <array>

namespace engine
{
    class SCUnitObj;

    namespace sc
    {
        class SCImageLibrary;
    }

    struct UnitEntry {
        wh_ptr<SCUnitObj> object;
        uint32_t serial = 0;
    };

    class MainGameScene :
        public Scene
    {
        ENTITY_INFO(MainGameScene, Scene)
    public:
        MainGameScene();
        virtual ~MainGameScene() override;

        virtual void Init() override;
        virtual void FrameStart() override;

		SCSim::Context* GetSCSimContext() const { return sc_sim_ctx_; }
		sc::SCImageLibrary* GetSCImageLibrary() const { return sc_image_library_.get(); }

    private:
        void PrepareMapLoader();
		void LoadResources();
        void SyncUnits();

    private:
        SCSim::Context* sc_sim_ctx_ = nullptr;
		float acc_delta_time_ = 0.0f; // for SCSimContext update

		const SCSim::UnitData* sc_units_ = {};
		int32_t sc_units_count_ = 0;

        // Unit ID - UnitEntry pair
		ankerl::unordered_dense::map<uint32_t, UnitEntry> unit_objects_;
		uint32_t sync_serial_ = 0;

		u_ptr<sc::SCImageLibrary> sc_image_library_ = {};
		MPQFileSystem mpq_file_system_ = {};
    };
}