// SCSimulatorAPI.h
#pragma once
#include <SCSimulator/Include/SCSimulatorTypes.h>

#include <cstdint>

#ifdef SCSIMULATOR_EXPORTS
#define SCSIM_API __declspec(dllexport)
#else
#define SCSIM_API __declspec(dllimport)
#endif

// TODO: 이후 변수 선별하여 SCSimulatorTypes.h 로 올릴것
namespace SCSim
{
	struct Context;

	constexpr uint32_t kInvalidUnitId = 0;

	struct UnitData {
		uint32_t id;
		int32_t  x;          // 시뮬레이션 위치(u->position)
		int32_t  y;
		int32_t  sprite_x;   // 렌더링 위치(u->sprite->position)
		int32_t  sprite_y;
		uint32_t flags;      // SCSimUnitFlags
	};
	static_assert(sizeof(UnitData) == 24, "UnitData layout changed. Must update FillUnit()");

	namespace UnitFlags {
		enum : uint32_t {
			None = 0,
			Hidden = 1u << 0,
		};
	}  // namespace UnitFlags

	constexpr size_t kMaxUnits = 1700;
	constexpr int kMaxActionPlayers = 8;  // action_state::selection 크기
	constexpr int kMaxSelection = 12;     // 선택 가능한 최대값(부대지정)

}; // namespace SCSim

extern "C" {

	// 실패 시 nullptr. 원인은 SCSim_GetLastError()로 확인
	SCSIM_API SCSim::Context* SCSim_Create(const char* mpq_dir);
	SCSIM_API void          SCSim_Destroy(SCSim::Context* ctx);

	// 실패 시 kInvalidUnitId. 맵 밖 좌표는 거부되지 않고 경계 안으로 보정됨
	SCSIM_API uint32_t SCSim_CreateUnit(SCSim::Context* ctx, SCSim::UnitType unit_type,
		SCSim::Player owner, int x, int y);

	// 같은 스레드에서 다음 SCSim_ 호출 전까지 유효. 실패한 호출만 덮어씀
	SCSIM_API const char* SCSim_GetLastError();

	SCSIM_API bool SCSim_LoadMap(SCSim::Context* ctx, const char* map_path, bool create_map_units);

	SCSIM_API bool SCSim_NextFrame(SCSim::Context* ctx);
	SCSIM_API int32_t SCSim_GetCurrentFrame(const SCSim::Context* ctx);

	// 부대 선택, 최대 12, 아무것도 선택되지 않았을 경우 0, 실패 시 -1. nullptr은 무시됨
	SCSIM_API int32_t SCSim_Select(SCSim::Context* ctx, SCSim::Player owner, const uint32_t* unit_ids, int count);

	SCSIM_API bool SCSim_Order(SCSim::Context* ctx, SCSim::Player owner, SCSim::OrderType order_type, int x, int y,
		uint32_t target_unit_id, int target_unit_type, bool queue);

	// visible, hidden 상태의 모든 유닛을 배열로 반환.
	SCSIM_API  int32_t SCSim_GetUnits(SCSim::Context* ctx, const SCSim::UnitData** out_units);

	// id의 유닛 정보를 *out_unit에 채움. 없거나(사망/무효) 실패하면 false
	// '숨겨진' 유닛도 조회 가능
	SCSIM_API bool SCSim_GetUnit(SCSim::Context* ctx, uint32_t unit_id,
		SCSim::UnitData* out_unit);
}