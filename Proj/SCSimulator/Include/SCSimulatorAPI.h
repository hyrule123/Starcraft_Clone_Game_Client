// SCSimulatorAPI.h
#pragma once
#include <SCSimulator/Include/SCSimulatorTypes.h>

#include <cstdint>

#ifdef SCSIMULATOR_EXPORTS
#define SCSIM_API __declspec(dllexport)
#else
#define SCSIM_API __declspec(dllimport)
#endif

constexpr uint32_t SCSIM_INVALID_UNIT_ID = 0;

struct SCSimUnit {
	uint32_t id;
	int32_t  x;          // 시뮬레이션 위치(u->position)
	int32_t  y;
	int32_t  sprite_x;   // 렌더링 위치(u->sprite->position)
	int32_t  sprite_y;
};
static_assert(sizeof(SCSimUnit) == 20, "SCSimUnit layout changed");

extern "C" {
	typedef struct SCSimContext SCSimContext;

	// 실패 시 nullptr. 원인은 SCSim_GetLastError()로 확인
	SCSIM_API SCSimContext* SCSim_Create(const char* mpq_dir);
	SCSIM_API void          SCSim_Destroy(SCSimContext* ctx);

	// 실패 시 SCSIM_INVALID_UNIT_ID. 맵 밖 좌표는 거부되지 않고 경계 안으로 보정됨
	SCSIM_API uint32_t SCSim_CreateUnit(SCSimContext* ctx, SCSimUnitType unit_type,
		SCSimPlayer owner, int x, int y);

	// 같은 스레드에서 다음 SCSim_ 호출 전까지 유효. 실패한 호출만 덮어씀
	SCSIM_API const char* SCSim_GetLastError();

	SCSIM_API bool SCSim_LoadMap(SCSimContext* ctx, const char* map_path, bool create_map_units);

	SCSIM_API bool SCSim_NextFrame(SCSimContext* ctx);
	SCSIM_API int32_t SCSim_GetCurrentFrame(const SCSimContext* ctx);

	// 부대 선택, 최대 12, 아무것도 선택되지 않았을 경우 0, 실패 시 -1. nullptr은 무시됨
	SCSIM_API int32_t SCSim_Select(SCSimContext* ctx, SCSimPlayer owner, const uint32_t* unit_ids, int count);

	SCSIM_API bool SCSim_Order(SCSimContext* ctx, SCSimPlayer owner, SCSimOrder order_type, int x, int y,
		uint32_t target_unit_id, int target_unit_type, bool queue);

	// 'visible' 상태의 모든 유닛을 배열로 반환.
	SCSIM_API  int32_t SCSim_GetUnits(SCSimContext* ctx, const SCSimUnit** out_units);

	// id의 유닛 정보를 *out_unit에 채움. 없거나(사망/무효) 실패하면 false
	// '숨겨진' 유닛도 조회 가능
	SCSIM_API bool SCSim_GetUnit(SCSimContext* ctx, uint32_t unit_id,
		SCSimUnit* out_unit);
}