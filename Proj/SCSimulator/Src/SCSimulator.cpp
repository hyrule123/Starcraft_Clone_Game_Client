#include <SCSimulator/Include/SCSimulatorAPI.h>

// Header-only library(OpenBW) for interacting with StarCraft: Brood War
#include <stdexcept>	// Since stdexcept is not included, must include it before using std::runtime_error
#include <SCSimulator/openbw/bwgame.h>
#include <SCSimulator/openbw/actions.h>

// Required STL for this source file
#include <string>
#include <array>

// 헤더에서 전역으로 선언했으므로 정의도 전역
struct SCSim::Context 
{
    explicit SCSim::Context(const char* mpq_dir) : player(mpq_dir) {}

    bwgame::game_player player;
    bwgame::action_state action_st;
    bool map_loaded = false;
    bool broken = false;

    std::array<UnitData, kMaxUnits> units{};  // GetUnits 결과 버퍼
};

namespace // Error
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
    bool CheckContext(const SCSim::Context* ctx, const char* func_name) {
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

	bool CheckOwnerRange(SCSim::Player owner, const char* func_name) {
		if ((int32_t)owner < 0 || (int32_t)SCSim::Player::kCount <= (int32_t)owner) {
			SetError(std::string(func_name) + ": invalid owner");
			return false;
		}
		return true;
	}

}  // namespace

namespace { // API related

    void FillUnit(const bwgame::action_functions& funcs, const bwgame::unit_t* u,
        SCSim::UnitData& out) {
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
        if (funcs.us_hidden(u)) { out.flags |= SCSim::UnitFlags::Hidden; }
    }

    // 상태를 읽기만 함. 검사가 끝난 이미지·스프라이트를 받음
    constexpr int kMaxPlayerColor = 15;  // tunit.pcx 16행
    bool FillImageInfo(const bwgame::state& st, const bwgame::sprite_t* sprite,
        const bwgame::image_t* image, SCSim::ImageInfo& out)
    {
        const int owner = sprite->owner;
        if (owner < 0 || owner >= static_cast<int>(st.players.size()))
        {
            SetError("sprite owner out of range: " + std::to_string(owner));
            return false;
        }

        const int color = st.players[owner].color;
        if (color < 0 || color > kMaxPlayerColor)
        {
            SetError("player color out of range: " + std::to_string(color) + " (owner " + std::to_string(owner) + ")");
            return false;
        }

        if (image->frame_index > UINT16_MAX)
        {
            SetError("frame index too large: " + std::to_string(image->frame_index));
            return false;
        }

        out = {};
        out.image_type = static_cast<uint16_t>(image->image_type->id);
        out.frame = static_cast<uint16_t>(image->frame_index);
        // 공중 유닛 둥실거림(iscript setvertpos)은 image->offset에 있으므로 반드시 더함
        out.x = sprite->position.x + image->offset.x;
        out.y = sprite->position.y + image->offset.y;
        out.flipped = (image->flags & bwgame::image_t::flag_horizontally_flipped) ? 1 : 0;
        out.player_color = static_cast<uint8_t>(color);
        out.hidden = ((sprite->flags & bwgame::sprite_t::flag_hidden) ||
            (image->flags & bwgame::image_t::flag_hidden)) ? 1 : 0;
        return true;
    }

}  // namespace

extern "C" {

    SCSim::Context* SCSim_Create(const char* mpq_dir) {
        if (!mpq_dir) {
            SetError("SCSim_Create: mpq_dir is null");
            return nullptr;
        }
        try {
            return new SCSim::Context(mpq_dir);
        }
        catch (const std::exception& e) {
            SetError(e.what());
        }
        catch (...) {
            SetError("SCSim_Create: unknown exception");
        }
        return nullptr;
    }

    void SCSim_Destroy(SCSim::Context* ctx) {
        delete ctx;  // nullptr이면 아무것도 하지 않음
    }

    const char* SCSim_GetLastError() {
        return last_error.c_str();
    }

    bool SCSim_LoadMap(SCSim::Context* ctx, const char* map_path,
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

    uint32_t SCSim_CreateUnit(SCSim::Context* ctx, SCSim::UnitType unit_type, SCSim::Player owner, int x, int y) {
		if (!CheckContext(ctx, "SCSim_CreateUnit")) {
			return SCSim::kInvalidUnitId;
		}

        if ((int32_t)unit_type < 0 || (int32_t)SCSim::UnitType::kCount <= (int32_t)unit_type) {
            SetError("SCSim_CreateUnit: invalid unit_type");
            return SCSim::kInvalidUnitId;
        }
        if ((int32_t)owner < 0 || (int32_t)SCSim::Player::kCount <= (int32_t)owner) {
            SetError("SCSim_CreateUnit: invalid owner");
            return SCSim::kInvalidUnitId;
        }
        try {
            auto& st = ctx->player.st();
            bwgame::action_functions funcs(st, ctx->action_st);
            const auto* type = funcs.get_unit_type(
                static_cast<bwgame::UnitTypes>(unit_type));
            
            st.last_error = 0;  // 이전 실패의 흔적 제거
            bwgame::unit_t* u = funcs.trigger_create_unit(type, { x, y }, (int)owner);
            return funcs.get_unit_id_32(u).raw_value;

            if (!u) {
                const char* reason = DescribeBWError(st.last_error);
                SetError(std::string("SCSim_CreateUnit: ") +
                    (reason ? reason : "creation rejected by simulation"));
                return SCSim::kInvalidUnitId;
            }
        }
        catch (const std::exception& e) {
            SetError(e.what());
        }
        catch (...) {
            SetError("SCSim_CreateUnit: unknown exception");
        }
        ctx->broken = true;
        return SCSim::kInvalidUnitId;
    }

    bool SCSim_NextFrame(SCSim::Context* ctx) {
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

	int32_t SCSim_GetCurrentFrame(const SCSim::Context* ctx)
    {
        if (!ctx || !ctx->map_loaded) {
            SetError("SCSim_GetCurrentFrame: ctx is null or map not loaded");
            return -1;
        }
        return static_cast<int32_t>(ctx->player.st().current_frame);
    }

    int32_t SCSim_Select(SCSim::Context* ctx, SCSim::Player owner, const uint32_t* unit_ids,
        int count) {
        if (!CheckContext(ctx, "SCSim_Select")) { return -1; } 
        if (!CheckOwnerRange(owner, "SCSim_Select")) { return -1; }

        if (count < 0 || count > SCSim::kMaxSelection) {
            SetError("SCSim_Select: count must be 0~12");
            return -1;
        }
        if (count > 0 && !unit_ids) {
            SetError("SCSim_Select: unit_ids is null");
            return -1;
        }
        try {
            bwgame::action_functions funcs(ctx->player.st(), ctx->action_st);
            std::array<bwgame::unit_t*, SCSim::kMaxSelection> units{};  // 남는 칸은 nullptr
            for (int i = 0; i < count; ++i) {
                units[i] = funcs.get_unit(bwgame::unit_id_32(unit_ids[i]));  // 무효면 nullptr
            }
            funcs.action_select((int)owner, units);  // nullptr은 OpenBW가 건너뜀
            return static_cast<int32_t>(ctx->action_st.selection[(int)owner].size());
        }
        catch (const std::exception& e) {
            SetError(e.what());
        }
        catch (...) {
            SetError("SCSim_Select: unknown exception");
        }
        return -1;  // 선택 목록만 건드리므로 broken 처리 안 함
    }

    bool SCSim_Order(SCSim::Context* ctx, SCSim::Player owner, SCSim::OrderType order_type, int x, int y,
        uint32_t target_unit_id, int target_unit_type, bool queue) {
        if (!CheckContext(ctx, "SCSim_Order")) { return false; }
		if (!CheckOwnerRange(owner, "SCSim_Order")) { return false; }

        bwgame::action_functions funcs(ctx->player.st(), ctx->action_st);
        const bwgame::order_type_t* order = nullptr;
        bwgame::unit_t* target = nullptr;
        const bwgame::unit_type_t* target_type = nullptr;

        // 1단계: 인자 해석. 상태를 건드리지 않으므로 실패해도 broken 아님
        try {
            order = funcs.get_order_type(static_cast<bwgame::Orders>(order_type));
            if (target_unit_id != SCSim::kInvalidUnitId) {
                target = funcs.get_unit(bwgame::unit_id_32(target_unit_id));
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
            if (!funcs.action_order((int)owner, order, { x, y }, target, target_type, queue)) {
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

    int32_t SCSim_GetUnits(SCSim::Context* ctx, const SCSim::UnitData** out_units) {
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
            auto append = [&](bwgame::unit_t* u) {
                FillUnit(funcs, u, ctx->units[count++]);
                };
            for (bwgame::unit_t* u : bwgame::ptr(st.visible_units)) { append(u); }
            for (bwgame::unit_t* u : bwgame::ptr(st.hidden_units)) { append(u); }

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

    bool SCSim_GetUnit(SCSim::Context* ctx, uint32_t unit_id, SCSim::UnitData* out_unit) {
        if (!out_unit) {
            SetError("SCSim_GetUnit: out_unit is null");
            return false;
        }
        if (!CheckContext(ctx, "SCSim_GetUnit")) return false;
        try {
            bwgame::action_functions funcs(ctx->player.st(), ctx->action_st);
            const bwgame::unit_t* u = funcs.get_unit(bwgame::unit_id_32(unit_id));
            if (!u) {
                SetError("SCSim_GetUnit: unit not found");
                return false;
            }
            FillUnit(funcs, u, *out_unit);
            return true;
        }
        catch (const std::exception& e) {
            SetError(e.what());
        }
        catch (...) {
            SetError("SCSim_GetUnit: unknown exception");
        }
        return false;
    }

    bool SCSim_GetUnitMainImage(SCSim::Context* ctx, uint32_t unit_id, SCSim::ImageInfo* out_info)
    {
        try
        {
            if (!CheckContext(ctx, "SCSim_GetUnitMainImage"))
            {
                return false;  // CheckContext가 메시지 설정
            }
            if (out_info == nullptr)
            {
                SetError("out_info is null");
                return false;
            }

			bwgame::action_functions funcs(ctx->player.st(), ctx->action_st);

            // 읽기 전용이라 broken 표시가 필요한 변경 단계는 없음
            const bwgame::unit_t* unit = funcs.get_unit(bwgame::unit_id_32(unit_id));
            if (unit == nullptr)
            {
                SetError("invalid or dead unit id: " + std::to_string(unit_id));
                return false;
            }

            // OpenBW가 검사하지 않는 포인터는 여기서 먼저 막음
            const bwgame::sprite_t* sprite = unit->sprite;
            if (sprite == nullptr || sprite->main_image == nullptr)
            {
                SetError("unit " + std::to_string(unit_id) + " has no sprite or main image");
                return false;
            }

            return FillImageInfo(funcs.st, sprite, sprite->main_image, *out_info);
        }
        catch (const std::exception& e)
        {
            SetError("exception: " + std::string(e.what()));
            return false;
        }
        catch (...)
        {
            SetError("unknown exception");
            return false;
        }
    }
        
	}  // extern "C"