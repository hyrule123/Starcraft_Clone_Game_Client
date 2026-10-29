// SCSimulatorAPI.h
#pragma once
#include <SCSimulator/Include/SCSimulatorEnums.h>
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

	// owner(0~7)의 선택을 unit_ids로 교체 (원작 Select 액션). count 0~kMaxSelection, 0이면 선택 해제
	// 입력은 Content가 정제해서 넘길 것 — 시뮬레이션은 무효 ID·핵미사일·숨김 유닛만 거름
	// 반환: 적용 후 선택 개수(0~kMaxSelection), 실패 시 -1
	SCSIM_API int32_t SCSim_Select(SCSim::Context* ctx, SCSim::Player owner, const uint32_t* unit_ids, int32_t count);

	// owner(0~7)의 현재 선택(사망 유닛 제외)을 owner별 컨텍스트 소유 버퍼에 채움
	// 포인터는 컨텍스트 수명 동안 유효, 내용은 같은 owner의 다음 GetSelection 호출 전까지 유효
	// Select·NextFrame 이후 자동 갱신되지 않음 — 다시 조회할 것
	// 반환: 개수(0~kMaxSelection), 실패 시 -1이며 *out_unit_ids = nullptr
	SCSIM_API int32_t SCSim_GetSelection(SCSim::Context* ctx, SCSim::Player owner, const uint32_t** out_unit_ids);

	// unit_ids[i]의 선택 판정 정보를 out_flags[i]에 기록 (SelectInfoFlags). 입력 순서 그대로, 필터링 없음
	// unit_ids와 out_flags는 모두 count칸. count 0~kMaxUnits (0이면 두 포인터 모두 nullptr 허용). 실패 시 false
	SCSIM_API bool SCSim_GetSelectInfo(SCSim::Context* ctx, const uint32_t* unit_ids, int32_t count, uint32_t* out_flags);

	// 현재 선택된 유닛들에게 명령 (owner 0~7)
	// 대상 유닛 없음: kInvalidUnitId, 대상 타입 없음: UnitType::None
	// 선택이 비었거나 시뮬레이션이 거부하면 false
	SCSIM_API bool SCSim_Order(SCSim::Context* ctx, SCSim::Player owner, SCSim::OrderType order_type, int32_t x, int32_t y,
		uint32_t target_unit_id, SCSim::UnitType target_unit_type, bool queue);

	// 현재 단일 선택된 유닛에게 건설 명령 (owner 0~7). 원작 Build 액션과 동일한 형식
	// order_type: PlaceBuilding / PlaceProtossBuilding / CreateProtossBuilding / DroneStartBuild /
	//             PlaceAddon / BuildNydusExit / BuildingLand 만 허용 (CTF 모드 오더는 지원하지 않음)
	// BuildingLand / BuildNydusExit는 unit_type이 선택 유닛 자신의 타입이어야 함
	// 애드온은 애드온 자신의 좌상단 타일 좌표를 넘김 (본체 위치는 시뮬레이션이 역산)
	// true = 명령 수락. 배치 불가·보급/자원 부족은 true를 반환하고 시뮬레이션이 무시함
	// 선택이 단일 유닛이 아니거나 해당 유닛이 그 건설을 할 수 없으면 false
	SCSIM_API bool SCSim_Build(SCSim::Context* ctx, SCSim::Player owner, SCSim::OrderType order_type,
		SCSim::UnitType unit_type, uint32_t tile_x, uint32_t tile_y);

	// 현재 단일 선택된 건물을 이륙 (owner 0~7). 원작 Lift 액션과 동일(위치는 건물 자신의 위치를 사용)
// 선택이 단일 유닛이 아니거나, 이륙할 수 없는 상태(건설·연구·업그레이드 중, 이륙 불가 유닛 등)면 false
	SCSIM_API bool SCSim_Liftoff(SCSim::Context* ctx, SCSim::Player owner);

	// visible, hidden 상태의 모든 유닛을 컨텍스트 소유 버퍼에 채움
	// 포인터는 컨텍스트 수명 동안 유효, 내용은 다음 NextFrame / GetUnits 호출 전까지 유효
	// 반환: 개수, 실패 시 -1이며 *out_units = nullptr
	SCSIM_API int32_t SCSim_GetUnits(SCSim::Context* ctx, const SCSim::UnitData** out_units);

	// 무효·사망 ID면 실패, hidden 유닛도 조회 가능(GetUnit과 동일)
	SCSIM_API bool SCSim_GetUnitVitals(SCSim::Context* ctx, uint32_t unit_id, SCSim::UnitVitals* out);

	// 살아 있는 스프라이트·이미지 전체(숨김 포함)를 컨텍스트 소유 버퍼에 채움
	// 스프라이트 순서는 정해져 있지 않음. 이미지는 스프라이트별로 OpenBW 목록 순서(앞 = 맨 위층)
	// 포인터는 컨텍스트 수명 동안 유효, 내용은 다음 NextFrame / GetSprites 호출 전까지 유효
	// 버퍼 오버플로(정상 상태에서는 발생하지 않음) 시 실패
	SCSIM_API bool SCSim_GetSprites(SCSim::Context* ctx, SCSim::SpriteSnapshot* out);

	// Sprite 타입 별로 sprites.dat 전체(sprite_type id = 인덱스)를 컨텍스트 소유 버퍼로, 포인터는 컨텍스트 수명 동안 유효
	// 데이터 버퍼의 크기는 크지 않으나, 1회 로드 시 불변 -> 초기화 시 1회만 호출
	// 성공 시 개수를 반환, 실패 시 -1
	SCSIM_API int32_t SCSim_GetSpriteTypes(SCSim::Context* ctx, const SCSim::SpriteTypeData** out); 

	// 플레이어 12명분(Player 순서)을 컨텍스트 소유 버퍼에 채움
	// 포인터는 컨텍스트 수명 동안 유효, 내용은 다음 GetPlayers 호출 전까지 유효
	// 반환: 개수, 실패 시 -1이며 *out_players = nullptr
	SCSIM_API int32_t SCSim_GetPlayers(SCSim::Context* ctx, const SCSim::PlayerData** out_players);

	// owner(0~7)의 동맹 설정 행 전체를 교체 (원작 동맹 메뉴 → set alliances 액션)
	// alliances: Player::kCount(12)개, 값은 AllianceState (0 적, 1 동맹, 2 동맹 승리)
	// 실패: 범위 밖 owner / count != 12 / 값 범위 밖 / 자기 자신·중립(11) 항목 변경 시도
	// (미구현) 맵·게임 설정에 따른 동맹 잠금 / 동맹 승리 허용 여부는 아직 검사하지 않음
	SCSIM_API bool SCSim_SetAlliances(SCSim::Context* ctx, SCSim::Player owner, const int32_t* alliances, int32_t count);

	// ---- 디버그 API ----
	// "입력은 명령 API로만" 원칙의 예외. 원작 트리거 액션으로 가능한 상태 변경만 허용

	// 플레이어 자원을 지정 값으로 설정 (owner 0~7). 맵 트리거 Set Resources(Set To)와 동일하게
	// 현재 보유량과 누적 채취량(점수용)을 같은 값으로 덮어씀. 음수는 실패
	SCSIM_API bool SCSim_Debug_SetResources(SCSim::Context* ctx, SCSim::Player owner, int32_t minerals, int32_t gas);

	// 맵 전체 타일(Megatile) 상태를 컨텍스트 소유 버퍼에 채움 (행 우선, index = y * width + x)
	// 포인터는 컨텍스트 수명 동안 유효, 내용은 다음 NextFrame / GetTiles 호출 전까지 유효
	// 성공 시 *out_width_megatile, *out_height_megatile = 메가타일 기준 맵 크기(1~kMaxMapMegatiles)
	// 실패 시 false, *out_megatile_states = nullptr, *out_width_megatile = *out_height_megatile = 0
	SCSIM_API bool SCSim_GetMegatileStates(SCSim::Context* ctx, const SCSim::MegatileState** out_megatile_states,
		uint32_t* out_width_megatile, uint32_t* out_height_megatile);
}