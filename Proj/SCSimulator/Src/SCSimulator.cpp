#include <SCSimulator/Include/SCSimulatorAPI.h>

// Header-only library(OpenBW) for interacting with StarCraft: Brood War
#include <stdexcept>	// Since stdexcept is not included, must include it before using std::runtime_error
#include <SCSimulator/openbw/bwgame.h>
#include <SCSimulator/openbw/actions.h>

// Required STL for this source file
#include <string>
#include <string_view>
#include <array>

// Windows.h 대신 필요한 두 함수만 선언 (OpenBW와 매크로 충돌 방지). 시그니처는 SDK와 동일
extern "C" __declspec(dllimport) void __stdcall OutputDebugStringA(const char* output_string);
extern "C" __declspec(dllimport) int __stdcall IsDebuggerPresent();
namespace // Assert
{
    // 메시지를 출력하고, 디버거가 붙어 있으면 true
    bool ReportAssert(const char* expr, std::string_view msg, const char* file, int line)
    {
        // "파일(줄): 내용" 형식이면 VS 출력 창에서 더블클릭으로 해당 줄로 이동
        std::string text = std::string(file) + "(" + std::to_string(line) + "): SCSIM_ASSERT failed: " + expr;
        if (!msg.empty())
        {
            text += " - ";
            text += msg;
        }
        text += "\n";

        OutputDebugStringA(text.c_str());
        return IsDebuggerPresent() != 0;
    }
}  // namespace

// 실패해도 진행은 계속됨. 같은 위치는 처음 한 번만 보고, msg는 실패했을 때만 평가
#ifndef NDEBUG
#define SCSIM_ASSERT_MSG(cond, msg) \
	do \
	{ \
		static bool scsim_assert_reported_ = false; \
		if (!(cond) && !scsim_assert_reported_) \
		{ \
			scsim_assert_reported_ = true; \
			if (ReportAssert(#cond, (msg), __FILE__, __LINE__)) \
			{ \
				__debugbreak(); \
			} \
		} \
	} while (false)
#else
#define SCSIM_ASSERT_MSG(cond, msg) do { (void)sizeof(cond); } while (false)
#endif

#define SCSIM_ASSERT(cond) SCSIM_ASSERT_MSG(cond, "")

// 헤더에서 전역으로 선언했으므로 정의도 전역
struct SCSim::Context 
{
    explicit SCSim::Context(const char* mpq_dir) : sim(mpq_dir) {}

	bwgame::game_player sim; // OpenBW game_player. 맵 로딩, 프레임 진행, 유닛 생성 등 대부분의 기능을 제공. 유저(player)를 뜻하는 게 아님!
    bwgame::action_state action_st;
    bool map_loaded = false;
    bool broken = false;

    std::array<UnitData, kMaxUnits> units{};  // GetUnits 결과 버퍼

    std::array<SCSim::SpriteData, SCSim::kMaxSprites> sprite_buffer = {};
    std::array<SCSim::ImageData, SCSim::kMaxImages> image_buffer = {};
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

namespace {
    void FillUnit(const bwgame::action_functions& funcs, const bwgame::unit_t* u,
        SCSim::UnitData& out) {
        out = {};
        out.id = funcs.get_unit_id_32(u).raw_value;
        out.x = u->position.x;
        out.y = u->position.y;

		out.sprite_index = u->sprite ? u->sprite->index : SCSim::kInvalidSpriteIndex;

        if (funcs.us_hidden(u)) { out.flags |= SCSim::UnitFlags::Hidden; }
    }

}  // namespace

namespace // ImageData 관련
{
    // ---- OpenBW 컨테이너 상한과 공개 상수 일치 검증 ----
    template<typename T>
    struct ContainerMaxSize;

    template<typename T, size_t N, size_t G>
    struct ContainerMaxSize<bwgame::object_container<T, N, G>>
    {
        static constexpr size_t value = N;
    };

    static_assert(ContainerMaxSize<decltype(bwgame::state::sprites_container)>::value == SCSim::kMaxSprites);
    static_assert(ContainerMaxSize<decltype(bwgame::state::images_container)>::value == SCSim::kMaxImages);
    static_assert(SCSim::kMaxImages - 1 <= UINT16_MAX);
    static_assert(std::tuple_size_v<decltype(bwgame::action_state::selection)> == SCSim::kMaxActionPlayers);
    static_assert(std::tuple_size_v<decltype(bwgame::action_state::control_groups)> == SCSim::kMaxActionPlayers);

    // ---- depth_key 비트 배치 (하위 → 상위) ----
    // [order 4][turret 1][mid 13][elevation 5] = 23비트
    // mid: 지상(elevation <= 4)은 position.y, 공중은 sprite index(원작의 포인터 주소 동점 처리 대체)
    constexpr uint32_t kOrderBits = 4;
    constexpr uint32_t kTurretBits = 1;
    constexpr uint32_t kMidBits = 13;
    constexpr uint32_t kElevationBits = 5;

    constexpr uint32_t kOrderShift = 0;
    constexpr uint32_t kTurretShift = kOrderShift + kOrderBits;
    constexpr uint32_t kMidShift = kTurretShift + kTurretBits;
    constexpr uint32_t kElevationShift = kMidShift + kMidBits;

    static_assert(kElevationShift + kElevationBits == SCSim::kDepthKeyBits);

    constexpr uint32_t Mask(uint32_t bits)
    {
        return (1u << bits) - 1u;
    }

    static_assert(SCSim::kMaxSprites - 1 <= Mask(kMidBits));

    constexpr int kGroundElevationMax = 4;	// 원작: elevation <= 4일 때만 y로 정렬
    constexpr int kMaxMapHeight = 1 << kMidBits;	// 8192px = 256타일

    // 스프라이트 단위 키 (이미지 순서 비트는 0)
    uint32_t MakeSpriteDepthBase(const bwgame::sprite_t& sprite)
    {
        SCSIM_ASSERT_MSG(0 <= sprite.elevation_level && sprite.elevation_level <= static_cast<int>(Mask(kElevationBits)),
            "Sprite elevation level out of range: " + std::to_string(sprite.elevation_level));

        const uint32_t elevation = static_cast<uint32_t>(
            std::clamp(sprite.elevation_level, 0, static_cast<int>(Mask(kElevationBits))));

        uint32_t mid = 0;
        if (sprite.elevation_level <= kGroundElevationMax)
        {
            // 맵 높이 <= 8192는 LoadMap에서 보장
            mid = static_cast<uint32_t>(std::max(sprite.position.y, 0)) & Mask(kMidBits);
        }
        else
        {
            mid = static_cast<uint32_t>(sprite.index) & Mask(kMidBits);
        }

        const uint32_t turret = (sprite.flags & bwgame::sprite_t::flag_turret) ? 1u : 0u;

        return (elevation << kElevationShift) | (mid << kMidShift) | (turret << kTurretShift);
    }

    // order: 스프라이트 안에서 그리는 순서 (0 = 가장 아래)
    void FillImageData(const bwgame::image_t& image, uint32_t sprite_depth_base, uint32_t order,
        SCSim::ImageData& out)
    {
		SCSIM_ASSERT_MSG(order <= Mask(kOrderBits), "image order out of range: " + std::to_string(order));

        const uint32_t clamped_order = std::min(order, Mask(kOrderBits));

        out.index = static_cast<uint16_t>(image.index);
        out.sprite_index = static_cast<uint16_t>(image.sprite->index);
        out.image_type = static_cast<uint16_t>(image.image_type->id);
        out.frame = static_cast<uint16_t>(image.frame_index);
        out.offset_x = static_cast<int16_t>(image.offset.x);
        out.offset_y = static_cast<int16_t>(image.offset.y);
        out.depth_key = sprite_depth_base | (clamped_order << kOrderShift);
        out.modifier = static_cast<uint8_t>(image.modifier);
        out.flags = (image.flags & bwgame::image_t::flag_horizontally_flipped)
            ? SCSim::ImageFlags::Flipped : SCSim::ImageFlags::None;
        out.reserved = 0;
    }

    uint8_t GetPlayerColor(const bwgame::state& st, int owner)
    {
        if (owner < 0 || owner >= static_cast<int>(st.players.size()))
        {
            SCSIM_ASSERT_MSG(false, "Sprite owner out of range: " + std::to_string(owner));
            return 0;
        }

        return static_cast<uint8_t>(st.players[owner].color);
    }
}

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
        if (ctx->broken)
        {
            SetError("SCSim_LoadMap: context is broken by a previous failure");
            return false;
        }
        if (ctx->map_loaded) {
            SetError("SCSim_LoadMap: map already loaded for this context");
            return false;
        }
        try {
			auto& player_st = ctx->sim.st();

            bwgame::game_load_functions load(player_st);
            load.load_map_file(map_path, [&]() {
                load.setup_info.create_no_units = !create_map_units;
                });

            if (player_st.game->map_height > kMaxMapHeight)
            {
                SetError("SCSim_LoadMap: map height exceeds 8192px (256 tiles)");
                ctx->broken = true;
                return false;
            }

            ctx->map_loaded = true;

            return true;
        }
        catch (const std::exception& e) {
            ctx->broken = true;
            SetError(e.what());
        }
        catch (...) {
            ctx->broken = true;
            SetError("SCSim_LoadMap: unknown exception");
        }
        return false;
    }

    uint32_t SCSim_CreateUnit(SCSim::Context* ctx, SCSim::UnitType unit_type, SCSim::Player owner, int x, int y)
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
        return static_cast<int32_t>(ctx->sim.st().current_frame);
    }

    int32_t SCSim_Select(SCSim::Context* ctx, SCSim::Player owner, const uint32_t* unit_ids,
        int count) {
        if (!CheckContext(ctx, "SCSim_Select")) { return -1; }
        
        // 촤대 Action Player = 8명
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
        if (!CheckContext(ctx, "SCSim_GetUnit")) { return false; }
        try {
            bwgame::action_functions funcs(ctx->sim.st(), ctx->action_st);
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

    SCSIM_API bool SCSim_GetSprites(SCSim::Context* ctx, SCSim::SpriteSnapshot* out)
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

        try
        {
            bwgame::state& st = ctx->sim.st();

            int32_t sprite_count = 0;
            int32_t image_count = 0;

            for (auto& line : st.sprites_on_tile_line)
            {
                for (bwgame::sprite_t& sprite : line)
                {
                    if (sprite.flags & bwgame::sprite_t::flag_hidden)
                    {
                        continue;
                    }

                    const int32_t first_image = image_count;
                    const uint32_t depth_base = MakeSpriteDepthBase(sprite);

                    // 원작은 images 리스트를 역순으로 그림 → 역순 순회 순서가 곧 아래 → 위
                    uint32_t order = 0;
                    for (auto it = sprite.images.rbegin(); it != sprite.images.rend(); ++it)
                    {
                        const bwgame::image_t& image = *it;
                        if (image.flags & bwgame::image_t::flag_hidden)
                        {
                            continue;
                        }

                        FillImageData(image, depth_base, order, ctx->image_buffer[image_count]);
                        ++image_count;
                        ++order;
                    }

                    // 보이는 이미지가 없으면 스프라이트도 제외 (엔진 쪽은 Disable)
                    if (image_count == first_image)
                    {
                        continue;
                    }

                    SCSim::SpriteData& sd = ctx->sprite_buffer[sprite_count];
                    sd.index = static_cast<uint16_t>(sprite.index);
                    sd.player_color = GetPlayerColor(st, sprite.owner);
                    sd.reserved = 0;
                    sd.x = sprite.position.x;
                    sd.y = sprite.position.y;
                    sd.first_image = static_cast<uint16_t>(first_image);
                    sd.image_count = static_cast<uint16_t>(image_count - first_image);
                    ++sprite_count;
                }
            }

            out->sprites = ctx->sprite_buffer.data();
            out->images = ctx->image_buffer.data();
            out->sprite_count = sprite_count;
            out->image_count = image_count;
            return true;
        }
        catch (const std::exception& e)
        {
            SetError(std::string("SCSim_GetSprites: ") + e.what());
            return false;
        }
        catch (...)
        {
            SetError("SCSim_GetSprites: unknown exception");
            return false;
        }
    }
        
	}  // extern "C"