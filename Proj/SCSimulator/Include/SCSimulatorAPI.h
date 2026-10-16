// SCSimulatorAPI.h
#pragma once
#include <SCSimulator/Include/SCSimulatorTypes.h>

#include <cstdint>

#ifdef SCSIMULATOR_EXPORTS
#define SCSIM_API __declspec(dllexport)
#else
#define SCSIM_API __declspec(dllimport)
#endif

extern "C" {

	// 실패 시 nullptr. 원인은 SCSim_GetLastError()로 확인
	SCSIM_API SCSim::Context* SCSim_Create(const char* mpq_dir);
	SCSIM_API void          SCSim_Destroy(SCSim::Context* ctx);

	// 실패 시 kInvalidUnitId. 맵 밖 좌표는 거부되지 않고 경계 안으로 보정됨
	SCSIM_API uint32_t SCSim_CreateUnit(SCSim::Context* ctx, SCSim::UnitType unit_type,
		SCSim::Player owner, int32_t x, int32_t y);

	// 같은 스레드에서 다음 SCSim_ 호출 전까지 유효. 실패한 호출만 덮어씀
	SCSIM_API const char* SCSim_GetLastError();

	SCSIM_API bool SCSim_LoadMap(SCSim::Context* ctx, const char* map_path, bool create_map_units);

	SCSIM_API bool SCSim_NextFrame(SCSim::Context* ctx);
	SCSIM_API int32_t SCSim_GetCurrentFrame(const SCSim::Context* ctx);

	// 부대 선택, 최대 12, 0~7번 플레이어만 가능. 아무것도 선택되지 않았을 경우 0, 실패 시 -1. nullptr인 유닛은 무시됨
	SCSIM_API int32_t SCSim_Select(SCSim::Context* ctx, SCSim::Player owner, const uint32_t* unit_ids, int32_t count);

	// 현재 선택된 유닛들에게 명령 (owner 0~7)
	// 대상 유닛 없음: kInvalidUnitId, 대상 타입 없음: UnitType::None
	// 선택이 비었거나 시뮬레이션이 거부하면 false
	SCSIM_API bool SCSim_Order(SCSim::Context* ctx, SCSim::Player owner, SCSim::OrderType order_type, int32_t x, int32_t y,
		uint32_t target_unit_id, SCSim::UnitType target_unit_type, bool queue);

	// visible, hidden 상태의 모든 유닛을 배열로 반환.
	SCSIM_API int32_t SCSim_GetUnits(SCSim::Context* ctx, const SCSim::UnitData** out_units);

	// 살아 있는 스프라이트·이미지 전체(숨김 포함)를 컨텍스트 소유 버퍼에 채움
	// 스프라이트 순서는 정해져 있지 않음. 이미지는 스프라이트별로 OpenBW 목록 순서(앞 = 맨 위층)
	// 포인터는 컨텍스트 수명 동안 유효, 내용은 다음 NextFrame / GetSprites 호출 전까지 유효
	// 버퍼 오버플로(정상 상태에서는 발생하지 않음) 시 실패
	SCSIM_API bool SCSim_GetSprites(SCSim::Context* ctx, SCSim::SpriteSnapshot* out);

	// 플레이어 12명분(Player 순서)을 컨텍스트 소유 버퍼로 반환. 실패 시 -1
	// 포인터는 컨텍스트 수명 동안 유효, 내용은 다음 GetPlayers 호출 전까지 유효
	SCSIM_API int32_t SCSim_GetPlayers(SCSim::Context* ctx, const SCSim::PlayerData** out_players);
}