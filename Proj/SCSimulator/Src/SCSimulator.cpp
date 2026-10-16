#include <SCSimulator/Include/SCSimulatorAPI.h>

// Header-only library(OpenBW) for interacting with StarCraft: Brood War
#include <stdexcept>	// Since stdexcept is not included, must include it before using std::runtime_error
#include <SCSimulator/openbw/bwgame.h>
#include <SCSimulator/openbw/actions.h>

// Required STL for this source file
#include <string>
#include <string_view>
#include <array>
#include <tuple>

// 헤더에서 전역으로 선언했으므로 정의도 전역
struct SCSim::Context 
{
    explicit Context(const char* mpq_dir) : sim(mpq_dir) {}

	bwgame::game_player sim; // OpenBW game_player. 맵 로딩, 프레임 진행, 유닛 생성 등 대부분의 기능을 제공. 유저(player)를 뜻하는 게 아님!
    bwgame::action_state action_st;
    bool map_loaded = false;
    bool broken = false;

    std::array<UnitData, kMaxUnits> units{};  // GetUnits 결과 버퍼

    std::array<SCSim::SpriteData, static_cast<size_t>(SCSim::kMaxSprites)> sprite_buffer = {};
    std::array<SCSim::ImageData, static_cast<size_t>(SCSim::kMaxImages)> image_buffer = {};
    std::array<SCSim::PlayerData, static_cast<size_t>(SCSim::Player::kCount)> player_buffer = {};
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

	bool CheckPlayerRange(SCSim::Player owner, const char* func_name) {
		int32_t owner_int = static_cast<int32_t>(owner);
		if (owner_int < 0 || owner_int >= static_cast<int32_t>(SCSim::Player::kCount)) {
			SetError(std::string(func_name) + ": invalid owner");
			return false;
		}
		return true;
	}

	bool CheckActionPlayerRange(SCSim::Player owner, const char* func_name) {
		int32_t owner_int = static_cast<int32_t>(owner);
        if(owner_int < 0 || owner_int >= SCSim::kMaxActionPlayers) {
			SetError(std::string(func_name) + ": invalid owner");
			return false;
		}
		return true;
	}
}  // namespace

namespace // Snapshot
{
    void FillImageData(const bwgame::image_t& image, SCSim::ImageData& out)
    {
        out = {};
        out.index = static_cast<uint32_t>(image.index);
        out.sprite_index = static_cast<uint32_t>(image.sprite->index);
        out.image_type = static_cast<uint32_t>(image.image_type->id);
        out.frame = static_cast<uint32_t>(image.frame_index);
        out.offset_x = image.offset.x;
        out.offset_y = image.offset.y;
        out.modifier = image.modifier;
        out.flags = static_cast<uint32_t>(image.flags);
    }

    void FillSpriteData(const bwgame::sprite_t& sprite, uint32_t first_image, uint32_t image_count,
        SCSim::SpriteData& out)
    {
        out = {};
        out.index = static_cast<uint32_t>(sprite.index);
        out.owner = sprite.owner;
        out.elevation_level = sprite.elevation_level;
        out.x = sprite.position.x;
        out.y = sprite.position.y;
        out.first_image = first_image;
        out.image_count = image_count;
        out.flags = static_cast<uint32_t>(sprite.flags);
    }
}  // namespace

namespace {
    void FillUnit(const bwgame::action_functions& funcs, const bwgame::unit_t* u,
        SCSim::UnitData& out) {
        out = {};
        out.id = funcs.get_unit_id_32(u).raw_value;
        out.x = u->position.x;
        out.y = u->position.y;

		out.sprite_index = static_cast<uint32_t>(u->sprite->index);

        out.status_flags = static_cast<uint32_t>(u->status_flags);
        out.sprite_flags = static_cast<uint32_t>(u->sprite->flags);
    }

}  // namespace

extern "C" {

    SCSim::Context* SCSim_Create(const char* mpq_dir) 
    {
        if (!mpq_dir) {
            SetError("SCSim_Create: mpq_dir is null");
            return nullptr;
        }
        try {
            return new SCSim::Context(mpq_dir);
        }
        catch (const std::exception& e) {
            SetError(std::string("SCSim_Create: ") + e.what());
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

    bool SCSim_LoadMap(SCSim::Context* ctx, const char* map_path, bool create_map_units)
    {
        if (!ctx || !map_path)
        {
            SetError("SCSim_LoadMap: ctx or map_path is null");
            return false;
        }
        if (ctx->broken)
        {
            SetError("SCSim_LoadMap: context is broken by a previous failure");
            return false;
        }
        if (ctx->map_loaded)
        {
            SetError("SCSim_LoadMap: map already loaded for this context");
            return false;
        }

        try
        {
            auto& player_st = ctx->sim.st();

            bwgame::game_load_functions load(player_st);
            load.load_map_file(map_path, [&]()
                {
                    // DIM 청크를 읽은 직후, 타일·지역 생성 전에 호출됨
                    // OpenBW는 크기 상한을 검사하지 않고, 256 초과 시 tile_region_index 범위 밖에 씀
                    const size_t tile_width = player_st.game->map_tile_width;
                    const size_t tile_height = player_st.game->map_tile_height;
                    if (tile_width > static_cast<size_t>(SCSim::kMaxMapTiles) || tile_height > static_cast<size_t>(SCSim::kMaxMapTiles))
                    {
                        throw std::runtime_error("map size " + std::to_string(tile_width) + "x" + std::to_string(tile_height) +
                            " exceeds " + std::to_string(SCSim::kMaxMapTiles) + "x" + std::to_string(SCSim::kMaxMapTiles) + " tiles");
                    }

                    load.setup_info.create_no_units = !create_map_units;
                });

            // COLR 청크는 setup_f 이후에 읽히므로 로드가 끝난 뒤 검사
            // 범위 밖 값은 저장만 될 뿐 메모리를 손상시키지 않으므로 사후 검사로 충분
            for (size_t i = 0; i < player_st.players.size(); ++i)
            {
                const int color = player_st.players[i].color;
                if (color < 0 || color >= SCSim::kMaxPlayerColors)
                {
                    SetError("SCSim_LoadMap: player " + std::to_string(i) + " color " + std::to_string(color) +
                        " is out of 1.16.1 range (0~" + std::to_string(SCSim::kMaxPlayerColors - 1) + ")");
                    ctx->broken = true;
                    return false;
                }
            }

            ctx->map_loaded = true;
            return true;
        }
        catch (const std::exception& e)
        {
            SetError(std::string("SCSim_LoadMap: ") + e.what());
        }
        catch (...)
        {
            SetError("SCSim_LoadMap: unknown exception");
        }

        ctx->broken = true;
        return false;
    }

    uint32_t SCSim_CreateUnit(SCSim::Context* ctx, SCSim::UnitType unit_type, SCSim::Player owner, int32_t x, int32_t y)
    {
        if (!CheckContext(ctx, "SCSim_CreateUnit"))
        {
            return SCSim::kInvalidUnitId;
        }
        if (static_cast<int32_t>(unit_type) < 0 ||
            static_cast<int32_t>(unit_type) >= static_cast<int32_t>(SCSim::UnitType::kCount))
        {
            SetError("SCSim_CreateUnit: invalid unit_type " + std::to_string(static_cast<int32_t>(unit_type)));
            return SCSim::kInvalidUnitId;
        }
        if (!CheckPlayerRange(owner, "SCSim_CreateUnit"))
        {
            return SCSim::kInvalidUnitId;
        }

        auto& st = ctx->sim.st();
        bwgame::action_functions funcs(st, ctx->action_st);
        const bwgame::unit_type_t* type = nullptr;

        // 1단계: 타입 조회와 사전 검사. 상태를 건드리지 않으므로 실패해도 broken 아님
        try
        {
            type = funcs.get_unit_type(static_cast<bwgame::UnitTypes>(unit_type));

            // trigger_create_unit이 last_error 없이 조용히 nullptr을 반환하는 경로를 미리 막음
            constexpr int kStareditPlaceable = 2;
            if ((type->staredit_availability_flags & kStareditPlaceable) == 0)
            {
                SetError("SCSim_CreateUnit: unit type " + std::to_string(static_cast<int32_t>(unit_type)) +
                    " is not placeable");
                return SCSim::kInvalidUnitId;
            }
        }
        catch (const std::exception& e)
        {
            SetError(std::string("SCSim_CreateUnit: ") + e.what());
            return SCSim::kInvalidUnitId;
        }
        catch (...)
        {
            SetError("SCSim_CreateUnit: unknown exception");
            return SCSim::kInvalidUnitId;
        }

        // 2단계: 실제 생성. 도중 예외는 절반만 진행된 상태일 수 있으므로 broken
        try
        {
            st.last_error = 0;  // 이전 실패의 흔적 제거
            bwgame::unit_t* u = funcs.trigger_create_unit(type, { x, y }, static_cast<int>(owner));

            if (!u)
            {
                const char* reason = DescribeBWError(st.last_error);
                if (!reason)
                {
                    // last_error 없는 실패: 건물 배치 불가 또는 place_completed_unit 실패(유닛은 kill_unit 처리됨)
                    reason = funcs.ut_building(type)
                        ? "building placement blocked or no space"
                        : "no space to place unit";
                }
                SetError(std::string("SCSim_CreateUnit: ") + reason);
                return SCSim::kInvalidUnitId;
            }

            return funcs.get_unit_id_32(u).raw_value;
        }
        catch (const std::exception& e)
        {
            SetError(std::string("SCSim_CreateUnit: ") + e.what());
        }
        catch (...)
        {
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
            bwgame::action_functions funcs(ctx->sim.st(), ctx->action_st);
            funcs.next_frame();
            return true;
        }
        catch (const std::exception& e) {
            SetError(std::string("SCSim_NextFrame: ") + e.what());
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
        return static_cast<int32_t>(ctx->sim.st().current_frame);
    }

    int32_t SCSim_Select(SCSim::Context* ctx, SCSim::Player owner, const uint32_t* unit_ids,
        int32_t count) {
        if (!CheckContext(ctx, "SCSim_Select")) { return -1; }
        
        // 최대 Action Player = 8명
        if (!CheckActionPlayerRange(owner, "SCSim_Select")) { return -1; }

        if (count < 0 || count > SCSim::kMaxSelection) {
            SetError("SCSim_Select: count must be 0~12");
            return -1;
        }
        if (count > 0 && !unit_ids) {
            SetError("SCSim_Select: unit_ids is null");
            return -1;
        }
        try {
            bwgame::action_functions funcs(ctx->sim.st(), ctx->action_st);
            std::array<bwgame::unit_t*, static_cast<size_t>(SCSim::kMaxSelection)> units{};  // 남는 칸은 nullptr
            for (int i = 0; i < count; ++i) {
                units[i] = funcs.get_unit(bwgame::unit_id_32(unit_ids[i]));  // 무효면 nullptr
            }
            funcs.action_select(static_cast<int>(owner), units);  // nullptr은 OpenBW가 건너뜀
            return static_cast<int32_t>(ctx->action_st.selection[static_cast<size_t>(owner)].size());
        }
        catch (const std::exception& e) {
            SetError(std::string("SCSim_Select: ") + e.what());
        }
        catch (...) {
            SetError("SCSim_Select: unknown exception");
        }
        return -1;  // 선택 목록만 건드리므로 broken 처리 안 함
    }
    bool SCSim_Order(SCSim::Context* ctx, SCSim::Player owner, SCSim::OrderType order_type, int32_t x, int32_t y,
        uint32_t target_unit_id, SCSim::UnitType target_unit_type, bool queue)
    {
        if (!CheckContext(ctx, "SCSim_Order"))
        {
            return false;
        }
        if (!CheckActionPlayerRange(owner, "SCSim_Order"))
        {
            return false;
        }

        // 범위 검사. OpenBW 예외에 맡기면 메시지가 내부 문구가 되므로 직접 거름
        const int32_t order_value = static_cast<int32_t>(order_type);
        if (order_value < 0 || order_value >= static_cast<int32_t>(SCSim::OrderType::kCount))
        {
            SetError("SCSim_Order: invalid order_type " + std::to_string(order_value));
            return false;
        }
        const int32_t target_type_value = static_cast<int32_t>(target_unit_type);
        if (target_type_value < 0 || target_type_value > static_cast<int32_t>(SCSim::UnitType::None))
        {
            SetError("SCSim_Order: invalid target_unit_type " + std::to_string(target_type_value));
            return false;
        }

        bwgame::action_functions funcs(ctx->sim.st(), ctx->action_st);
        const bwgame::order_type_t* order = nullptr;
        bwgame::unit_t* target = nullptr;
        const bwgame::unit_type_t* target_type = nullptr;

        // 1단계: 인자 해석. 상태를 건드리지 않으므로 실패해도 broken 아님
        try
        {
            order = funcs.get_order_type(static_cast<bwgame::Orders>(order_type));

            if (target_unit_id != SCSim::kInvalidUnitId)
            {
                target = funcs.get_unit(bwgame::unit_id_32(target_unit_id));
                if (!target)
                {
                    SetError("SCSim_Order: target unit not found " + std::to_string(target_unit_id));
                    return false;
                }
            }

            // None(228)은 OpenBW get_unit_type이 예외를 던지므로 조회하지 않음
            if (target_unit_type != SCSim::UnitType::None)
            {
                target_type = funcs.get_unit_type(static_cast<bwgame::UnitTypes>(target_unit_type));
            }
        }
        catch (const std::exception& e)
        {
            SetError(std::string("SCSim_Order: ") + e.what());
            return false;
        }
        catch (...)
        {
            SetError("SCSim_Order: unknown exception");
            return false;
        }

        // 2단계: 실제 명령. 도중 실패 시 일부 유닛만 명령을 받았을 수 있음
        try
        {
            if (!funcs.action_order(static_cast<int>(owner), order, { x, y }, target, target_type, queue))
            {
                SetError("SCSim_Order: order rejected by simulation");
                return false;
            }
            return true;
        }
        catch (const std::exception& e)
        {
            SetError(std::string("SCSim_Order: ") + e.what());
        }
        catch (...)
        {
            SetError("SCSim_Order: unknown exception");
        }

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
            auto& st = ctx->sim.st();
            bwgame::action_functions funcs(st, ctx->action_st);

            bool overflow = false;

            size_t count = 0;
            auto append = [&](bwgame::unit_t* u) {
                // 유닛 컨테이너 상한 = 버퍼 크기라 넘칠 수 없지만, 버퍼 쓰기 전에 확인
                if (count >= ctx->units.size())
                {
                    overflow = true;
                    return;
                }
                FillUnit(funcs, u, ctx->units[count++]);
                };
            for (bwgame::unit_t* u : bwgame::ptr(st.visible_units)) { append(u); }
            for (bwgame::unit_t* u : bwgame::ptr(st.hidden_units)) { append(u); }

            if (overflow)
            {
				SetError("SCSim_GetUnits: unit buffer overflow");
				return -1;
            }

            *out_units = ctx->units.data();
            return static_cast<int32_t>(count);
        }
        catch (const std::exception& e) {
            SetError(std::string("SCSim_GetUnits: ") + e.what());
        }
        catch (...) {
            SetError("SCSim_GetUnits: unknown exception");
        }
        return -1;
    }

    bool SCSim_GetSprites(SCSim::Context* ctx, SCSim::SpriteSnapshot* out)
    {
        if (!CheckContext(ctx, "SCSim_GetSprites"))
        {
            return false;
        }
        if (out == nullptr)
        {
            SetError("SCSim_GetSprites: out is null");
            return false;
        }
        *out = {};

        try
        {
            const bwgame::state& st = ctx->sim.st();

            size_t sprite_count = 0;
            size_t image_count = 0;

            // 살아 있는 스프라이트는 숨김 여부와 관계없이 정확히 한 타일 라인에 들어 있음
            for (const auto& line : st.sprites_on_tile_line)
            {
                for (const bwgame::sprite_t& sprite : line)
                {
                    // 컨테이너 상한 = 버퍼 크기라 넘칠 수 없지만, 버퍼 쓰기 전에 확인
                    if (sprite_count >= ctx->sprite_buffer.size())
                    {
                        SetError("SCSim_GetSprites: sprite buffer overflow");
                        return false;
                    }

                    const size_t first_image = image_count;
                    for (const bwgame::image_t& image : sprite.images)
                    {
                        if (image_count >= ctx->image_buffer.size())
                        {
                            SetError("SCSim_GetSprites: image buffer overflow");
                            return false;
                        }
                        FillImageData(image, ctx->image_buffer[image_count]);
                        ++image_count;
                    }

                    FillSpriteData(sprite, static_cast<uint32_t>(first_image),
                        static_cast<uint32_t>(image_count - first_image), ctx->sprite_buffer[sprite_count]);
                    ++sprite_count;
                }
            }

            out->sprites = ctx->sprite_buffer.data();
            out->images = ctx->image_buffer.data();
            out->sprite_count = static_cast<int32_t>(sprite_count);
            out->image_count = static_cast<int32_t>(image_count);
            return true;
        }
        catch (const std::exception& e)
        {
            SetError(std::string("SCSim_GetSprites: ") + e.what());
        }
        catch (...)
        {
            SetError("SCSim_GetSprites: unknown exception");
        }
        return false;
    }

    int32_t SCSim_GetPlayers(SCSim::Context* ctx, const SCSim::PlayerData** out_players)
    {
        if (!out_players)
        {
            SetError("SCSim_GetPlayers: out_players is null");
            return -1;
        }
        *out_players = nullptr;
        if (!CheckContext(ctx, "SCSim_GetPlayers"))
        {
            return -1;
        }

        const auto& players = ctx->sim.st().players;
        for (size_t i = 0; i < players.size(); ++i)
        {
            ctx->player_buffer[i] = {};
            ctx->player_buffer[i].color = players[i].color;
        }

        *out_players = ctx->player_buffer.data();
        return static_cast<int32_t>(players.size());
    }
        
	}  // extern "C"



    namespace //type traits
    {
        template<typename T>
        struct ContainerMaxSize;

        template<typename T, size_t N, size_t G>
        struct ContainerMaxSize<bwgame::object_container<T, N, G>>
        {
            static constexpr size_t value = N;
        };

        template<typename T>
        struct StaticVectorCapacity;

        template<typename T, size_t N>
        struct StaticVectorCapacity<bwgame::static_vector<T, N>>
        {
            static constexpr size_t value = N;
        };

        static_assert(ContainerMaxSize<decltype(bwgame::state::units_container)>::value == SCSim::kMaxUnits);
        static_assert(ContainerMaxSize<decltype(bwgame::state::sprites_container)>::value == SCSim::kMaxSprites);
        static_assert(ContainerMaxSize<decltype(bwgame::state::images_container)>::value == SCSim::kMaxImages);
        using SelectionArray = decltype(bwgame::action_state::selection);
        static_assert(std::tuple_size_v<SelectionArray> == SCSim::kMaxActionPlayers);
        static_assert(StaticVectorCapacity<SelectionArray::value_type>::value == SCSim::kMaxSelection);
        static_assert(std::tuple_size_v<decltype(bwgame::state::players)> == static_cast<size_t>(SCSim::Player::kCount));


        // ---- 플래그 타입 검증 ----
        // 1) 공개 enum과 OpenBW enum의 기반 타입이 같은지
        static_assert(std::is_same_v<
            std::underlying_type_t<decltype(SCSim::SpriteFlags::None)>,
            std::underlying_type_t<bwgame::sprite_t::flags_t>>);
        static_assert(std::is_same_v<
            std::underlying_type_t<decltype(SCSim::ImageFlags::None)>,
            std::underlying_type_t<bwgame::image_t::flags_t>>);

        // 2) 실제 복사 대상인 flags 필드가 int이고, 경계 타입(uint32_t)에 손실 없이 담기는지
        static_assert(std::is_same_v<decltype(bwgame::sprite_t::flags), int>);
        static_assert(std::is_same_v<decltype(bwgame::image_t::flags), int>);
        static_assert(sizeof(int) <= sizeof(uint32_t));

        // 3) 경계 구조체 필드 타입이 공개 enum 기반 타입과 같은지
        static_assert(std::is_same_v<decltype(SCSim::SpriteData::flags),
            std::underlying_type_t<decltype(SCSim::SpriteFlags::None)>>);
        static_assert(std::is_same_v<decltype(SCSim::ImageData::flags),
            std::underlying_type_t<decltype(SCSim::ImageFlags::None)>>);
        static_assert(std::is_same_v<decltype(bwgame::sprite_t::index), size_t>);
        static_assert(std::is_same_v<decltype(bwgame::image_t::index), size_t>);
        static_assert(static_cast<uint32_t>(SCSim::kMaxSprites) - 1 < SCSim::kInvalidSpriteIndex);
        static_assert(SCSim::SpriteFlags::Turret == bwgame::sprite_t::flag_turret);
        static_assert(SCSim::SpriteFlags::Hidden == bwgame::sprite_t::flag_hidden);
        static_assert(SCSim::SpriteFlags::Burrowed == bwgame::sprite_t::flag_burrowed);
        static_assert(SCSim::SpriteFlags::IscriptNoBrk == bwgame::sprite_t::flag_iscript_nobrk);

        static_assert(SCSim::ImageFlags::HorizontallyFlipped == bwgame::image_t::flag_horizontally_flipped);
        static_assert(SCSim::ImageFlags::YFrozen == bwgame::image_t::flag_y_frozen);
        static_assert(SCSim::ImageFlags::HasDirectionalFrames == bwgame::image_t::flag_has_directional_frames);
        static_assert(SCSim::ImageFlags::HasIscriptAnimations == bwgame::image_t::flag_has_iscript_animations);
        static_assert(SCSim::ImageFlags::Clickable == bwgame::image_t::flag_clickable);
        static_assert(SCSim::ImageFlags::Hidden == bwgame::image_t::flag_hidden);
        static_assert(SCSim::ImageFlags::UsesSpecialOffset == bwgame::image_t::flag_uses_special_offset);

        // unit_t::status_flags 원본 비트 일치 검증
        static_assert(SCSim::UnitStatusFlags::Completed == bwgame::unit_t::status_flag_completed);
        static_assert(SCSim::UnitStatusFlags::GroundedBuilding == bwgame::unit_t::status_flag_grounded_building);
        static_assert(SCSim::UnitStatusFlags::Flying == bwgame::unit_t::status_flag_flying);
        static_assert(SCSim::UnitStatusFlags::Burrowed == bwgame::unit_t::status_flag_burrowed);
        static_assert(SCSim::UnitStatusFlags::InBunker == bwgame::unit_t::status_flag_in_bunker);
        static_assert(SCSim::UnitStatusFlags::Loaded == bwgame::unit_t::status_flag_loaded);
        static_assert(SCSim::UnitStatusFlags::RequiresDetector == bwgame::unit_t::status_flag_requires_detector);
        static_assert(SCSim::UnitStatusFlags::Cloaked == bwgame::unit_t::status_flag_cloaked);
        static_assert(SCSim::UnitStatusFlags::Disabled == bwgame::unit_t::status_flag_disabled);
        static_assert(SCSim::UnitStatusFlags::PassivelyCloaked == bwgame::unit_t::status_flag_passively_cloaked);
        static_assert(SCSim::UnitStatusFlags::OrderNotInterruptible == bwgame::unit_t::status_flag_order_not_interruptible);
        static_assert(SCSim::UnitStatusFlags::IscriptNoBrk == bwgame::unit_t::status_flag_iscript_nobrk);
        static_assert(SCSim::UnitStatusFlags::CannotAttack == bwgame::unit_t::status_flag_cannot_attack);
        static_assert(SCSim::UnitStatusFlags::CanTurn == bwgame::unit_t::status_flag_can_turn);
        static_assert(SCSim::UnitStatusFlags::CanMove == bwgame::unit_t::status_flag_can_move);
        static_assert(SCSim::UnitStatusFlags::Collision == bwgame::unit_t::status_flag_collision);
        static_assert(SCSim::UnitStatusFlags::Immovable == bwgame::unit_t::status_flag_immovable);
        static_assert(SCSim::UnitStatusFlags::GroundUnit == bwgame::unit_t::status_flag_ground_unit);
        static_assert(SCSim::UnitStatusFlags::NoCollide == bwgame::unit_t::status_flag_no_collide);
        static_assert(SCSim::UnitStatusFlags::Gathering == bwgame::unit_t::status_flag_gathering);
        static_assert(SCSim::UnitStatusFlags::TurretWalking == bwgame::unit_t::status_flag_turret_walking);
        static_assert(SCSim::UnitStatusFlags::Invincible == bwgame::unit_t::status_flag_invincible);
        static_assert(SCSim::UnitStatusFlags::ReadyToAttack == bwgame::unit_t::status_flag_ready_to_attack);
        static_assert(SCSim::UnitStatusFlags::SpeedUpgrade == bwgame::unit_t::status_flag_speed_upgrade);
        static_assert(SCSim::UnitStatusFlags::CooldownUpgrade == bwgame::unit_t::status_flag_cooldown_upgrade);
        static_assert(SCSim::UnitStatusFlags::Hallucination == bwgame::unit_t::status_flag_hallucination);
        static_assert(SCSim::UnitStatusFlags::LifetimeExpired == bwgame::unit_t::status_flag_lifetime_expired);

        static_assert(std::is_same_v<decltype(SCSim::UnitData::sprite_flags),
            std::underlying_type_t<decltype(SCSim::SpriteFlags::None)>>);
    }