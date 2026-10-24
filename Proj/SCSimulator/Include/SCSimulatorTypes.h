#pragma once
#include <cstdint>
#include <type_traits>
#include <limits>

#include <SCSimulator/Include/SCSimulatorEnums.h>

namespace SCSim
{
	struct Context;	// Forward

	// 경계 타입 규칙
	// - OpenBW size_t 인덱스(슬롯 번호·프레임 번호) → uint32_t. 축소는 DLL Fill 함수에서만 일어나며,
	//   값의 상한(컨테이너 크기, iscript 프레임 범위)은 DLL의 static_assert·assert로 보장
	//   (GPU 인스턴스 데이터가 32비트라 어차피 한 번은 축소가 필요 — 그 지점을 DLL 한 곳으로 모음)
	// - OpenBW int(게임 값: 플레이어 번호·색상·좌표 등) → int32_t
	// - 비트 플래그는 OpenBW int 필드를 uint32_t로 명시 변환(상수에 0x80000000이 있어 signed로 표현 불가)

	constexpr int32_t kMaxUnits = 1700;			// OpenBW units_container 상한
	constexpr int32_t kMaxMapTiles = 256;		// 1.16.1 맵 최대 크기(타일), OpenBW tile_region_index도 256x256 고정
	constexpr int32_t kMaxPlayerColors = 16;	// 1.16.1 플레이어 색상 수(tunit.pcx 16행)
	constexpr int32_t kMaxActionPlayers = 8;	// 명령 주체 수, OpenBW action_state::selection 크기
	constexpr int32_t kMaxSelection = 12;		// 동시 선택 상한(부대지정), OpenBW selection 원소 용량
	constexpr int32_t kMaxSprites = 2500;		// OpenBW sprites_container 상한
	constexpr int32_t kMaxImages = 5000;		// OpenBW images_container 상한

	constexpr uint32_t kInvalidSpriteIndex = std::numeric_limits<uint32_t>::max();
	constexpr uint32_t kInvalidUnitId = 0;

	// image_t::modifier (images.dat "draw function"). 비트 플래그가 아닌 단일 상태값.
	// 이름은 커뮤니티 명칭(PyMS 등) 기준, 괄호는 OpenBW 렌더러의 실제 처리
	enum class ImageModifier : int32_t
	{
		Normal = 0,
		NormalNoHallu = 1,   // 0과 동일하게 그림
		Cloaking = 2,   // 은폐 중 (trans50 + 밝기 선택)
		Cloaked = 3,   // 은폐 (trans50)
		Decloaking = 4,   // 은폐 해제 중 (2와 같은 그리기)
		EnemyCloaking = 5,   // OpenBW 렌더러 미지원
		EnemyCloaked = 6,   // OpenBW 렌더러 미지원
		EnemyDecloaking = 7,   // OpenBW 렌더러 미지원
		Distortion = 8,   // EMP 등 왜곡
		Remapping = 9,   // images.dat remapping LUT
		Shadow = 10,
		HpBar = 11,  // OpenBW 렌더러 미지원
		WarpTexture = 12,
		SelectionCircle = 13,  // OpenBW 렌더러 미지원
		PlayerColor = 14,  // 생성 시 data1 = owner. OpenBW 렌더러 미지원
		UpdateRect = 15,  // OpenBW 렌더러 미지원
		Hallucination = 16,  // OpenBW 렌더러 미지원
		WarpFlash = 17,
	};

	// sprite_t::flags 원본 비트 (1.16.1 배치). 아래에 없는 비트는 계약에 포함되지 않음
	// 0x8(selected)은 OpenBW에서 설정되지 않아 항상 0 — 선택 정보로 사용 금지
	namespace SpriteFlags
	{
		enum : uint32_t
		{
			None = 0,
			Turret = 0x10,
			Hidden = 0x20,
			Burrowed = 0x40,
			IscriptNoBrk = 0x80,	// iscript nobrkcodestart~end 구간 (애니메이션 중단 불가)
		};
	}

	struct SpriteData
	{
		uint32_t index;				// sprite_t::index (슬롯 번호, 0도 유효)
		int32_t  owner;				// sprite_t::owner (0~11). 색상은 SCSim_GetPlayers로 조회
		int32_t  elevation_level;	// sprite_t::elevation_level
		int32_t  x;					// sprite_t::position
		int32_t  y;
		uint32_t first_image;		// SpriteSnapshot::images 내 시작 위치
		uint32_t image_count;		// OpenBW 목록 순서(앞 = 맨 위층)로 연속 배치. 숨김 이미지 포함
		uint32_t flags;				// SpriteFlags (sprite_t::flags 비트 그대로)
	};
	static_assert(sizeof(SpriteData) == 32);

	// image_t::flags 원본 비트 (1.16.1 배치). 아래에 없는 비트는 계약에 포함되지 않음
	// 0x1(redraw)은 OpenBW에서 해제되지 않아 의미 없음 — 사용 금지
	namespace ImageFlags
	{
		enum : uint32_t
		{
			None = 0,
			HorizontallyFlipped = 0x2,
			YFrozen = 0x4,
			HasDirectionalFrames = 0x8,
			HasIscriptAnimations = 0x10,
			Clickable = 0x20,
			Hidden = 0x40,
			UsesSpecialOffset = 0x80,
		};
	}

	struct ImageData
	{
		uint32_t index;				// image_t::index (슬롯 번호, 0도 유효)
		uint32_t sprite_index;		// 소속 sprite_t::index
		int32_t image_type;		// image_type->id (images.dat 번호)
		uint32_t frame;				// image_t::frame_index
		int32_t  offset_x;			// image_t::offset (스프라이트 기준 로컬 좌표)
		int32_t  offset_y;
		int32_t  frozen_y_value;	// flags의 YFrozen가 켜졌을 때만 유효
		int32_t  modifier;			// image_t::modifier
		uint32_t flags;				// ImageFlags (image_t::flags 비트 그대로)
		int32_t  modifier_data1;	// image_t::modifier_data1 상태에 따른 색상 변화 플래그
	};
	static_assert(sizeof(ImageData) == 40);


	struct PlayerData
	{
		int32_t color;			// player_t::color (LoadMap에서 0~15 보장)
		int32_t minerals;		// state::current_minerals
		int32_t gas;			// state::current_gas
	};
	static_assert(sizeof(PlayerData) == 12);

	static_assert(std::is_trivially_copyable_v<PlayerData>&& std::is_standard_layout_v<PlayerData>);
	static_assert(std::is_trivially_copyable_v<SpriteData>&& std::is_standard_layout_v<SpriteData>);
	static_assert(std::is_trivially_copyable_v<ImageData>&& std::is_standard_layout_v<ImageData>);

	struct SpriteSnapshot
	{
		const SpriteData* sprites;
		const ImageData* images;
		int32_t sprite_count;
		int32_t image_count;
	};

	struct UnitData
	{
		uint32_t id;				// unit_id_32::raw_value
		UnitType unit_type;			// u->unit_type->id
		int32_t  x;					// unit_t::position
		int32_t  y;
		uint32_t sprite_index;		// u->sprite->index
		uint32_t status_flags;		// unit_t::status_flags 비트 그대로 (UnitStatusFlags)
		uint32_t sprite_flags;		// u->sprite->flags 비트 그대로 (SpriteFlags) — 숨김은 SpriteFlags::Hidden
	};
	static_assert(sizeof(UnitData) == 28);
	static_assert(std::is_trivially_copyable_v<UnitData>&& std::is_standard_layout_v<UnitData>);

	// SCSim_GetSelectInfo 결과. 0 = 무효 ID(사망, 세대 불일치)
	namespace SelectInfoFlags
	{
		enum : uint32_t
		{
			None = 0,
			Valid = 0x1,			// 살아 있는 유닛
			MultiSelectable = 0x2,	// action_functions::unit_can_be_multi_selected
		};
	}

	// unit_t::status_flags 원본 비트 (1.16.1 배치). 아래에 없는 비트는 계약에 포함되지 않음
	// 0x8, 0x4000, 0x400000은 OpenBW에서도 의미 미상 — 사용 금지
	namespace UnitStatusFlags
	{
		enum : uint32_t
		{
			None = 0,
			Completed = 0x1,
			GroundedBuilding = 0x2,
			Flying = 0x4,
			Burrowed = 0x10,
			InBunker = 0x20,
			Loaded = 0x40,
			RequiresDetector = 0x100,
			Cloaked = 0x200,
			Disabled = 0x400,
			PassivelyCloaked = 0x800,
			OrderNotInterruptible = 0x1000,
			IscriptNoBrk = 0x2000,
			CannotAttack = 0x8000,
			CanTurn = 0x10000,
			CanMove = 0x20000,
			Collision = 0x40000,
			Immovable = 0x80000,
			GroundUnit = 0x100000,
			NoCollide = 0x200000,
			Gathering = 0x800000,
			TurretWalking = 0x1000000,
			Invincible = 0x4000000,
			ReadyToAttack = 0x8000000,
			SpeedUpgrade = 0x10000000,
			CooldownUpgrade = 0x20000000,
			Hallucination = 0x40000000,
			LifetimeExpired = 0x80000000,
		};
	}

	enum class Player : int32_t {
		_1 = 0,
		_2 = 1,
		_3 = 2,
		_4 = 3,
		_5 = 4,
		_6 = 5,
		_7 = 6,
		_8 = 7,
		_9 = 8,
		_10 = 9,
		_11 = 10,
		Neutral = 11,
		kCount
	};



}