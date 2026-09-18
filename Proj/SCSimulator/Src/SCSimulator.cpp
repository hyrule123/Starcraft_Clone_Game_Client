#include <SCSimulator/Include/SCSimulatorAPI.h>

// Header-only library(OpenBW) for interacting with StarCraft: Brood War
#include <stdexcept>	// Since stdexcept is not included, must include it before using std::runtime_error
#include <SCSimulator/openbw/bwgame.h>

// SCSimulator/Src/SCSimulatorAPI.cpp
#include <stdexcept>
#include <string>

#include <SCSimulator/Include/SCSimulatorAPI.h>
#include <SCSimulator/openbw/bwgame.h>
#include <SCSimulator/openbw/actions.h>

// 헤더에서 전역으로 선언했으므로 정의도 전역
struct SCSimContext {
    explicit SCSimContext(const char* mpq_dir) : player(mpq_dir) {}

    bwgame::game_player player;
    bwgame::action_state action_st;
    bool map_loaded = false;
};

namespace {

    thread_local std::string last_error;

    // OpenBW의 st.last_error 값 → 메시지. 원작 에러 번호 기반
    const char* DescribeBWError(int code) {
        switch (code) {
        case 60: return "no free position to place unit";  // 주석 없음, 코드 흐름으로 추정
        case 61: return "cannot create more units";
        case 62: return "unable to create unit";
        default: return nullptr;  // 0 또는 알 수 없는 코드
        }
    }

    void SetError(std::string msg) { last_error = std::move(msg); }

    constexpr int kUnitTypeCount = 228;  // bwgame::get_unit_type의 상한
    constexpr int kMaxPlayers = 12;

}  // namespace

extern "C" {

    SCSimContext* SCSim_Create(const char* mpq_dir) {
        if (!mpq_dir) {
            SetError("SCSim_Create: mpq_dir is null");
            return nullptr;
        }
        try {
            return new SCSimContext(mpq_dir);
        }
        catch (const std::exception& e) {
            SetError(e.what());
        }
        catch (...) {
            SetError("SCSim_Create: unknown exception");
        }
        return nullptr;
    }

    void SCSim_Destroy(SCSimContext* ctx) {
        delete ctx;  // nullptr이면 아무것도 하지 않음
    }

    const char* SCSim_GetLastError() {
        return last_error.c_str();
    }

    bool SCSim_LoadMap(SCSimContext* ctx, const char* map_path,
        bool create_map_units) {
        if (!ctx || !map_path) {
            SetError("SCSim_LoadMap: ctx or map_path is null");
            return false;
        }
        if (ctx->map_loaded) {
            SetError("SCSim_LoadMap: map already loaded for this context");
            return false;
        }
        try {
            bwgame::game_load_functions load(ctx->player.st());
            load.load_map_file(map_path, [&]() {
                load.setup_info.create_no_units = !create_map_units;
                });
            ctx->map_loaded = true;
            return true;
        }
        catch (const std::exception& e) {
            SetError(e.what());
        }
        catch (...) {
            SetError("SCSim_LoadMap: unknown exception");
        }
        return false;
    }

    uint32_t SCSim_CreateUnit(SCSimContext* ctx, int unit_type, int owner,
        int x, int y) {
        if (!ctx) {
            SetError("SCSim_CreateUnit: ctx is null");
            return SCSIM_INVALID_UNIT_ID;
        }
        if (!ctx->map_loaded) {
            SetError("SCSim_CreateUnit: map not loaded");
            return SCSIM_INVALID_UNIT_ID;
        }
        if (unit_type < 0 || unit_type >= kUnitTypeCount) {
            SetError("SCSim_CreateUnit: invalid unit_type");
            return SCSIM_INVALID_UNIT_ID;
        }
        if (owner < 0 || owner >= kMaxPlayers) {
            SetError("SCSim_CreateUnit: invalid owner");
            return SCSIM_INVALID_UNIT_ID;
        }
        try {
            auto& st = ctx->player.st();
            bwgame::action_functions funcs(st, ctx->action_st);
            const auto* type = funcs.get_unit_type(
                static_cast<bwgame::UnitTypes>(unit_type));
            
            st.last_error = 0;  // 이전 실패의 흔적 제거
            bwgame::unit_t* u = funcs.trigger_create_unit(type, { x, y }, owner);
            return funcs.get_unit_id_32(u).raw_value;

            if (!u) {
                const char* reason = DescribeBWError(st.last_error);
                SetError(std::string("SCSim_CreateUnit: ") +
                    (reason ? reason : "creation rejected by simulation"));
                return SCSIM_INVALID_UNIT_ID;
            }
        }
        catch (const std::exception& e) {
            SetError(e.what());
        }
        catch (...) {
            SetError("SCSim_CreateUnit: unknown exception");
        }
        return SCSIM_INVALID_UNIT_ID;
    }

}  // extern "C"