#pragma once
#include <Engine/Core/CoreMinimal.h>

#include <Content/HLSL/SCMap/SCMapBaker.hlsli>

namespace engine
{
	class Texture2D;
	class Texture2DArray;
	class TypedBuffer;
	class ConstantBuffer;
}

namespace sc
{
	struct TileSetData;
	struct TileSetDataGPU;

#pragma pack(push, 1)

	// ===================================================================
	// StarCraft 타일셋 데이터 구조
	//
	// 파싱 흐름 (MTXM 값 하나 → 32x32 픽셀):
	//   MTXM[y*w+x] ─ 그룹/타일 분해 ─→ CV5[group].megatile[tile]
	//               ─→ VX4[mega].VX4_minitiles[0..15]   (미니타일 4x4)
	//               ─→ VR4[ref].pixel[8][8]        (팔레트 색인)
	//               ─→ WPE[index]                  (최종 RGB)
	//
	// 단위:  메가타일 32x32px  =  미니타일(8x8px) 4x4개
	//        DIM/MTXM은 모두 메가타일 단위
	// ===================================================================

	// -------------------------------------------------------------------
	// CV5 : 타일 그룹 정의. MTXM이 참조하는 최상위 단위.
	//   파일 크기 / 52 = 그룹 개수 (타일셋마다 다름)
	//   ID 범위 0~2047
	//   terrain_type == 1 이면 두다드 그룹이라 오프셋 4~19의 해석이 다름.
	//   단, megatile[16]은 양쪽 모두 오프셋 20으로 동일 → 렌더링은 구분 불필요.
	// -------------------------------------------------------------------
	
	struct CV5
	{
		uint16_t terrain_type;      // 0=미사용, 1=두다드, 2+=지형/가장자리
		uint16_t flags;

		uint16_t edge_left;         // 인접 가능한 타일 타입
		uint16_t edge_up;
		uint16_t edge_right;
		uint16_t edge_down;

		uint16_t piece_left;        // 항상 0
		uint16_t piece_up;          // 다중 타일 블록 판정
		uint16_t piece_right;       // 항상 0
		uint16_t piece_down;

		// VX4 / VF4 megatile lookup table.
		std::array<uint16_t, kCV5MegatilesPerGroup> megatiles; // VX4/VF4 인덱스
	};
	static_assert(sizeof(CV5) == 52, "CV5 must be 52 bytes");



	// -------------------------------------------------------------------
	// VX4 : 메가타일당 미니타일 "그래픽" 참조
	//   bit 0       : 수평 반전 ★ 놓치면 조각이 좌우로 뒤집힘
	//   상위 15비트 : VR4 인덱스 → (v >> 1) 로 추출  (& 0xFFFE 아님)
	//   배열 순서   : 좌상단 → 우하단 (m%4 = x, m/4 = y)
	// -------------------------------------------------------------------

	constexpr size_t minitiles_per_VX4 = 16;
	struct VX4
	{
		using Elem = uint16_t;
		std::array<Elem, minitiles_per_VX4> minitiles;
	};
	static_assert(sizeof(VX4) == 32, "VX4 must be 32 bytes");
	constexpr uint16_t VX4Ref(VX4::Elem v) { return v >> 1; }
	constexpr bool   VX4Flipped(VX4::Elem v) { return (v & 0b0000'0000'0000'0001) != 0; }

	// -------------------------------------------------------------------
	// VF4 : 메가타일당 미니타일 "속성" (길찾기·시야). 렌더링에는 미사용.
	//   길찾기 격자는 메가타일이 아니라 미니타일 해상도 → 맵 크기의 4배
	// -------------------------------------------------------------------
	struct VF4
	{
		using Elem = uint16_t;
		std::array<Elem, 16> minitile_flags;
	};
	static_assert(sizeof(VF4) == 32, "VF4 must be 32 bytes");

	namespace VF4Flag
	{
		constexpr uint16_t kWalkable = 1 << 0;
		constexpr uint16_t kMid = 1 << 1;
		constexpr uint16_t kHigh = 1 << 2;   // Mid, High 둘 다 0이면 Low
		constexpr uint16_t kBlocksView = 1 << 3;
		constexpr uint16_t kRamp = 1 << 4;
	}

	// -------------------------------------------------------------------
	// VR4 : 미니타일 8x8 픽셀 데이터. 값은 WPE 색인(0~255).
	//   타일셋 파일 중 가장 큼 → 로딩 시간의 대부분
	// -------------------------------------------------------------------
	constexpr size_t minile_pixel_nrow = 8;
	constexpr size_t minile_pixel_ncol = 8;
	constexpr size_t minile_pixel_count = minile_pixel_nrow * minile_pixel_ncol;
	struct VR4
	{
		using Elem = uint8_t;
		std::array<std::array<Elem, minile_pixel_ncol>, minile_pixel_nrow> pixels;        // [y][x]
	};
	static_assert(sizeof(VR4) == 64, "VR4 must be 64 bytes");

	// -------------------------------------------------------------------
	// WPE : 256색 팔레트. 파일 전체가 항상 1024바이트.
	//   ★ 4번째 바이트는 알파가 아니라 미사용(항상 0).
	//     그대로 알파로 쓰면 텍스처가 전부 투명해짐 → 직접 255를 채울 것
	//   런타임에 색 순환(물 일렁임 등)으로 변경되는 대상
	// -------------------------------------------------------------------
	struct WPE
	{
		uint8_t r, g, b, unused;    // unused는 알파가 아님 (항상 0)
	};
	static_assert(sizeof(WPE) == 4, "WPE must be 4 bytes");

#pragma pack(pop)


	class SCTileSetPcxLUT;

	//스타크래프트 맵 데이터 속 "ERA" 안에 들어있는 정보 
	enum class TileSetType : uint32_t
	{
		kBadlands = 0x00,
		kSpace_platform = 0x01,
		kInstallation = 0x02,
		kAshworld = 0x03,
		kJungle = 0x04,
		kDesert = 0x05,
		kIce = 0x06,
		kTwilight = 0x07,
		kCount = 0x08
	};

	constexpr std::array<std::string_view, (size_t)TileSetType::kCount> kTileSetTypeNames = {
		"badlands",
		"platform",
		"install",
		"ashworld",
		"jungle",
		"Desert",
		"Ice",
		"Twilight"
	};

	//맵데이터로부터 로드해야하는 데이터들의 플래그
	enum class SCChunkType
	{
		kTerrain,
		kMapSize,
		kTileMapAtlas,
		kUnitPlacement,
		kEND
	};

	constexpr uint32_t ChunkIDByteStride = 4u;

	// 글자 수가 적을 경우 '띄어쓰기' 포함
	// 길이는 '4' 고정 -> uint32_t 등으로 캐스팅해서 비교 가능
	constexpr std::string_view SCChunkTypeName[(uint32_t)SCChunkType::kEND] = {
	"ERA ", "DIM ", "MTXM", "UNIT"
	};

	constexpr uint32_t ChunkID(const std::string_view s)
	{
		if (s.size() != 4) throw "4 chars required";

		return (uint32_t)(uint8_t)s[0] | ((uint32_t)(uint8_t)s[1] << 8)
			| ((uint32_t)(uint8_t)s[2] << 16) | ((uint32_t)(uint8_t)s[3] << 24);
	}

	constexpr std::string IDToChunkName(const uint32_t id)
	{
		std::string ret;
		ret.resize(4);

		ret[0] = (char)(id & 0xFF);
		ret[1] = (char)((id >> 8) & 0xFF);
		ret[2] = (char)((id >> 16) & 0xFF);
		ret[3] = (char)((id >> 24) & 0xFF);

		return ret;
	}

	// 4글자 char -> uint32_t로 변환
	constexpr uint32_t map_data_type_name_IDs[(uint32_t)SCChunkType::kEND] = {
		ChunkID(SCChunkTypeName[0]),
		ChunkID(SCChunkTypeName[1]),
		ChunkID(SCChunkTypeName[2]),
		ChunkID(SCChunkTypeName[3])
	};

#pragma pack(push, 1)
	struct UnitData
	{
		uint32_t unit_info;
		uint16_t pos_x;
		uint16_t pos_y;
		uint16_t id;          //유닛 ID
		uint16_t linked_unit_flag;         //애드온, 나이더스 커널 등 연결된 유닛 유무
		uint16_t property;    //유닛 속성(아래 참고 - 아직 사용하지는 않음)

		uint16_t upgradable_property;  //유닛의 상태변화에 적용가능한 속성(체력통 증가, 마나통 증가 등등) - 사용 X

		uint8_t owner;
		uint8_t hp_percent;
		uint8_t shield_percent;
		uint8_t energy_percent;

		uint32_t resources;
		uint16_t num_unit_in_container;

		uint16_t unit_state_flag;
		uint32_t unused;
		uint32_t linked_unit_class_inst; //연결된 유닛(애드온, 나이더스 커널)의 인스턴스 정보
	};
#pragma pack(pop)
	static_assert(sizeof(UnitData) == 36, "UnitData size must be 36 bytes.");

	/*
* https://cafe.daum.net/rpgguild/6cWR/158
	0000 = 유닛에 적용가능한 속성

	xxxx xxxx xxxE DCBA(2)



	A - 클로킹 칸 허용
	B - 버로우 칸 허용
	C - 띄우기 칸 허용
	D - 환상 칸 허용
	E - 무적 칸 허용
	x - 알려지지 않음 / 사용되지 않음



	0000 = 유닛의 현재상태값의 수치변화에 적용가능한 기능

	xxxx xxxx xxFE DCBA(2)



	A - 유닛 소유 기능
	B - 체력 변화 기능
	C - 쉴드 변화 기능
	D - 마나 변화 기능
	E - 자원의 양 변화 기능
	F - 적재소 양 변화 가능(리버, 캐리어)
	x - 알려지지 않음 / 사용되지 않음
*/

// MTXM : 좌상단으로부터 한 칸씩 메가타일의 인덱스가 적혀있음.
	using MTXM = uint16_t;

	struct RGBA { uint8_t r, g, b, a; };

	constexpr uint32_t MTXMGroup(uint16_t v) { return (v & 0b0111'1111'1111'0000) >> 4; }
	constexpr uint32_t MTXMTile(uint16_t v) { return  v & 0b0000'0000'0000'1111; }

	// 최종 결과물
	struct SCMap
	{
		TileSetType terrain_type = TileSetType::kCount;
		uint32_t megatile_width = 0;
		uint32_t megatile_height = 0;
		
		std::vector<UnitData> unit_data = {};

		// R16_UINT, 메가타일 수, MTXM 원값(그룹 << 4 | 타일)
		std::shared_ptr<engine::TypedBuffer> mtxm = {};

		// 타일셋 공유 (맵 PS에서 Creep 메가타일 디코딩용)
		std::shared_ptr<engine::TypedBuffer> cv5_megatiles = {};
		std::shared_ptr<engine::TypedBuffer> vx4_minitiles = {};
		std::shared_ptr<engine::TypedBuffer> vr4_minitile_wpe_indices = {};

		// WPE 색상 팔레트
		std::shared_ptr<engine::TypedBuffer> wpe_color_palettes = {};
		std::shared_ptr<engine::TypedBuffer> cloak_fade_selector = {};

		// modifier별 팔레트 룩업 테이블 묶음과 묶음별 색인 오프셋
		std::shared_ptr<engine::TypedBuffer> pcx_tileset_LUT = {};
		std::shared_ptr<engine::TypedBuffer> pcx_tileset_LUT_offsets = {};

		// Creep
		std::shared_ptr<engine::Texture2D> creep_random_tiles = {};		// R8_UINT, 메가타일 크기. 타일별 Creep 바닥 번호(0 ~ kCreepTileCount - 1)
		std::shared_ptr<engine::Texture2DArray> creep_edge = {};		// 타일셋 공유. 테두리 GRP(R8G8_UINT, 32x32 × kCreepEdgeFrameCount)
	};

	struct SCMapLoadingData
	{
		TileSetType terrain_type = TileSetType::kCount;
		uint32_t megatile_width = 0;
		uint32_t megatile_height = 0;

		std::vector<uint8_t> map_raw_bytes = {};
		engine::StringHashTable<std::vector<uint8_t>> map_data_table = {};

		std::vector<MTXM> mtxm = {};
		std::vector<UnitData> unit_data = {};

		const TileSetData* tileset = {};
		const TileSetDataGPU* tileset_gpu = {};
	};

	struct TileSetData
	{
		std::vector<CV5> cv5 = {};
		std::vector<VX4> vx4 = {};
		std::vector<VF4> vf4 = {};
		std::vector<VR4> vr4 = {};
		std::vector<WPE> wpe = {};

		std::shared_ptr<SCTileSetPcxLUT> pcx_lut = {};
		std::vector<uint8_t> creep_grp_bytes = {};	// GPU에서 생성한 뒤 버림. 비어있을 수 있음.
	};

	struct TileSetDataGPU
	{
		std::shared_ptr<engine::TypedBuffer> CV5_megatiles = {};	// from CV5
		std::shared_ptr<engine::TypedBuffer> VX4_minitiles = {};	// from VX4
		std::shared_ptr<engine::TypedBuffer> VR4_minitile_wpe_indices = {}; // from VR4
		std::shared_ptr<engine::TypedBuffer> WPE_color_palettes = {}; // from WPE, std::shared_ptr - 렌더링할 때 필요
		std::shared_ptr<engine::TypedBuffer> PCX_tileset_LUT = {}; // from SCTileSetPcxLUT, std::shared_ptr - 렌더링할 때 필요
		std::shared_ptr<engine::TypedBuffer> PCX_tileset_LUT_offsets = {}; // from SCTileSetPcxLUT, std::shared_ptr - 렌더링할 때 필요
		std::shared_ptr<engine::TypedBuffer> cloak_fade_selector = {};
		std::shared_ptr<engine::Texture2DArray> creep_edge = {};
	};

}