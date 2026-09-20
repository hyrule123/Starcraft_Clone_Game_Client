#include <SCSimulator/Include/SCSimulatorAPI.h>

// Header-only library(OpenBW) for interacting with StarCraft: Brood War
#include <stdexcept>	// Since stdexcept is not included, must include it before using std::runtime_error
#include <SCSimulator/openbw/bwgame.h>
#include <SCSimulator/openbw/actions.h>

// Required STL for this source file
#include <string>
#include <array>

namespace // Constants
{
    constexpr size_t kMaxUnits = 1700;
    constexpr int kUnitTypeCount = 228;  // bwgame::get_unit_type의 상한
    constexpr int kMaxPlayers = 12;        // 플레이어 수 상한, 0 ~ 7은 사용자, 나머지는 중립 건물 또는 AI, ...
    constexpr int kMaxActionPlayers = 8;  // action_state::selection 크기
    constexpr int kMaxSelection = 12;     // 선택 가능한 최대값(부대지정)
}

// 헤더에서 전역으로 선언했으므로 정의도 전역
struct SCSimContext 
{
    explicit SCSimContext(const char* mpq_dir) : player(mpq_dir) {}

    bwgame::game_player player;
    bwgame::action_state action_st;
    bool map_loaded = false;
    bool broken = false;

    std::array<SCSimUnit, kMaxUnits> units{};  // GetUnits 결과 버퍼
};

namespace 
{
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

    // ctx가 쓸 수 있는 상태인지 검사. 실패 시 에러 메시지를 남기고 false
    bool CheckContext(const SCSimContext* ctx, const char* func_name) {
        if (!ctx) {
            SetError(std::string(func_name) + ": ctx is null");
            return false;
        }
        if (ctx->broken) {
            SetError(std::string(func_name) + ": context is broken by a previous failure");
            return false;
        }
        if (!ctx->map_loaded) {
            SetError(std::string(func_name) + ": map not loaded");
            return false;
        }
        return true;
    }

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
		if (!CheckContext(ctx, "SCSim_CreateUnit")) {
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

    bool SCSim_NextFrame(SCSimContext* ctx) {
		if (!CheckContext(ctx, "SCSim_NextFrame")) {
			return false;
		}

        try {
            bwgame::action_functions funcs(ctx->player.st(), ctx->action_st);
            funcs.next_frame();
            return true;
        }
        catch (const std::exception& e) {
            SetError(e.what());
        }
        catch (...) {
            SetError("SCSim_NextFrame: unknown exception");
        }
        ctx->broken = true;
        return false;
    }

	int32_t SCSim_GetCurrentFrame(const SCSimContext* ctx)
    {
        if (!ctx || !ctx->map_loaded) {
            SetError("SCSim_GetCurrentFrame: ctx is null or map not loaded");
            return -1;
        }
        return static_cast<int32_t>(ctx->player.st().current_frame);
    }

    int32_t SCSim_Select(SCSimContext* ctx, int owner, const uint32_t* unit_ids,
        int count) {
        if (!CheckContext(ctx, "SCSim_Select")) { return -1; } 
        if (owner < 0 || owner >= kMaxActionPlayers) {
            SetError("SCSim_Select: invalid owner");
            return -1;
        }
        if (count < 0 || count > kMaxSelection) {
            SetError("SCSim_Select: count must be 0~12");
            return -1;
        }
        if (count > 0 && !unit_ids) {
            SetError("SCSim_Select: unit_ids is null");
            return -1;
        }
        try {
            bwgame::action_functions funcs(ctx->player.st(), ctx->action_st);
            std::array<bwgame::unit_t*, kMaxSelection> units{};  // 남는 칸은 nullptr
            for (int i = 0; i < count; ++i) {
                units[i] = funcs.get_unit(bwgame::unit_id_32(unit_ids[i]));  // 무효면 nullptr
            }
            funcs.action_select(owner, units);  // nullptr은 OpenBW가 건너뜀
            return static_cast<int32_t>(ctx->action_st.selection[owner].size());
        }
        catch (const std::exception& e) {
            SetError(e.what());
        }
        catch (...) {
            SetError("SCSim_Select: unknown exception");
        }
        return -1;  // 선택 목록만 건드리므로 broken 처리 안 함
    }

    bool SCSim_Order(SCSimContext* ctx, int owner, int order_type, int x, int y,
        uint32_t target_id, int target_unit_type, bool queue) {
        if (!CheckContext(ctx, "SCSim_Order")) { return false; }

        if (owner < 0 || owner >= kMaxActionPlayers) 
        {
            SetError("SCSim_Order: invalid owner");
            return false;
        }

        bwgame::action_functions funcs(ctx->player.st(), ctx->action_st);
        const bwgame::order_type_t* order = nullptr;
        bwgame::unit_t* target = nullptr;
        const bwgame::unit_type_t* target_type = nullptr;

        // 1단계: 인자 해석. 상태를 건드리지 않으므로 실패해도 broken 아님
        try {
            order = funcs.get_order_type(static_cast<bwgame::Orders>(order_type));
            if (target_id != SCSIM_INVALID_UNIT_ID) {
                target = funcs.get_unit(bwgame::unit_id_32(target_id));
                if (!target) {
                    SetError("SCSim_Order: target unit not found");
                    return false;
                }
            }
            if (target_unit_type >= 0) {
                target_type = funcs.get_unit_type(
                    static_cast<bwgame::UnitTypes>(target_unit_type));
            }
        }
        catch (const std::exception& e) {
            SetError(e.what());
            return false;
        }
        catch (...) {
            SetError("SCSim_Order: unknown exception");
            return false;
        }

        // 2단계: 실제 명령. 도중 실패 시 일부 유닛만 명령을 받았을 수 있음
        try {
            if (!funcs.action_order(owner, order, { x, y }, target, target_type, queue)) {
                SetError("SCSim_Order: order rejected by simulation");
                return false;
            }
            return true;
        }
        catch (const std::exception& e) {
            SetError(e.what());
        }
        catch (...) {
            SetError("SCSim_Order: unknown exception");
        }

        // 여기 오면 실패
        ctx->broken = true;
        return false;
    }

    int32_t SCSim_GetUnits(SCSimContext* ctx, const SCSimUnit** out_units) {
        if (!out_units) {
            SetError("SCSim_GetUnits: out_units is null");
            return -1;
        }
        *out_units = nullptr;
        if (!CheckContext(ctx, "SCSim_GetUnits")) return -1;

        try {
            auto& st = ctx->player.st();
            bwgame::action_functions funcs(st, ctx->action_st);
            size_t count = 0;
            for (bwgame::unit_t* u : bwgame::ptr(st.visible_units)) {
                if (count == kMaxUnits) {  // 이론상 불가능, 방어용
                    SetError("SCSim_GetUnits: unit buffer overflow");
                    return -1;
                }
                SCSimUnit& out = ctx->units[count++];
                out.id = funcs.get_unit_id_32(u).raw_value;
                out.x = u->position.x;
                out.y = u->position.y;
                if (u->sprite) {
                    out.sprite_x = u->sprite->position.x;
                    out.sprite_y = u->sprite->position.y;
                }
                else {
                    out.sprite_x = out.x;
                    out.sprite_y = out.y;
                }
            }
            *out_units = ctx->units.data();
            return static_cast<int32_t>(count);
        }
        catch (const std::exception& e) {
            SetError(e.what());
        }
        catch (...) {
            SetError("SCSim_GetUnits: unknown exception");
        }
        return -1;
    }
        
	}  // extern "C"