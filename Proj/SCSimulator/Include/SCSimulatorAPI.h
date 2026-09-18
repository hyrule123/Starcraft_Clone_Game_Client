// SCSimulatorAPI.h
#pragma once
#include <cstdint>

#ifdef SCSIMULATOR_EXPORTS
#define SCSIM_API __declspec(dllexport)
#else
#define SCSIM_API __declspec(dllimport)
#endif

constexpr uint32_t SCSIM_INVALID_UNIT_ID = 0;

extern "C" {
	typedef struct SCSimContext SCSimContext;

	// 실패 시 nullptr. 원인은 SCSim_GetLastError()로 확인
	SCSIM_API SCSimContext* SCSim_Create(const char* mpq_dir);
	SCSIM_API void          SCSim_Destroy(SCSimContext* ctx);

	// 실패 시 SCSIM_INVALID_UNIT_ID. 맵 밖 좌표는 거부되지 않고 경계 안으로 보정됨
	SCSIM_API uint32_t SCSim_CreateUnit(SCSimContext* ctx, int unit_type,
		int owner, int x, int y);

	// 같은 스레드에서 다음 SCSim_ 호출 전까지 유효. 실패한 호출만 덮어씀
	SCSIM_API const char* SCSim_GetLastError();

	SCSIM_API bool SCSim_LoadMap(SCSimContext* ctx, const char* map_path, bool create_map_units);
}