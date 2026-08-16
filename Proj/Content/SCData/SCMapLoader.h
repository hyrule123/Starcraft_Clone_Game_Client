#pragma once
#include <Engine/Core/CoreMinimal.h>
#include <Engine/Core/Math.h>

#include <array>

namespace engine
{
#pragma pack(push, 1)

// ===================================================================
// StarCraft 타일셋 데이터 구조
//
// 파싱 흐름 (MTXM 값 하나 → 32x32 픽셀):
//   MTXM[y*w+x] ─ 그룹/타일 분해 ─→ CV5[group].megatile[tile]
//               ─→ VX4[mega].minitile[0..15]   (미니타일 4x4)
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
		uint16 terrain_type;      // 0=미사용, 1=두다드, 2+=지형/가장자리
		uint16 flags;

		uint16 edge_left;         // 인접 가능한 타일 타입
		uint16 edge_up;
		uint16 edge_right;
		uint16 edge_down;

		uint16 piece_left;        // 항상 0
		uint16 piece_up;          // 다중 타일 블록 판정
		uint16 piece_right;       // 항상 0
		uint16 piece_down;

		uint16 megatile[16];      // VX4/VF4 인덱스
	};
	static_assert(sizeof(CV5) == 52, "CV5 must be 52 bytes");

// -------------------------------------------------------------------
// VX4 : 메가타일당 미니타일 "그래픽" 참조
//   bit 0       : 수평 반전 ★ 놓치면 조각이 좌우로 뒤집힘
//   상위 15비트 : VR4 인덱스 → (v >> 1) 로 추출  (& 0xFFFE 아님)
//   배열 순서   : 좌상단 → 우하단 (m%4 = x, m/4 = y)
// -------------------------------------------------------------------
	struct VX4
	{
		uint16 minitile[16];      // 4x4, 좌상단 → 우하단
	};
	static_assert(sizeof(VX4) == 32, "VX4 must be 32 bytes");
	constexpr uint16 VX4Ref(uint16 v) { return v >> 1; }
	constexpr bool   VX4Flipped(uint16 v) { return (v & 0b0000'0000'0000'0001) != 0; }

// -------------------------------------------------------------------
// VF4 : 메가타일당 미니타일 "속성" (길찾기·시야). 렌더링에는 미사용.
//   길찾기 격자는 메가타일이 아니라 미니타일 해상도 → 맵 크기의 4배
// -------------------------------------------------------------------
	struct VF4
	{
		uint16 flags[16];
	};
	static_assert(sizeof(VF4) == 32, "VF4 must be 32 bytes");

	namespace VF4Flag
	{
		constexpr uint16 kWalkable = 0x0001;
		constexpr uint16 kMid = 0x0002;
		constexpr uint16 kHigh = 0x0004;   // Mid, High 둘 다 0이면 Low
		constexpr uint16 kBlocksView = 0x0008;
		constexpr uint16 kRamp = 0x0010;
	}

// -------------------------------------------------------------------
// VR4 : 미니타일 8x8 픽셀 데이터. 값은 WPE 색인(0~255).
//   타일셋 파일 중 가장 큼 → 로딩 시간의 대부분
// -------------------------------------------------------------------
	struct VR4
	{
		uint8 pixel[8][8];        // [y][x]
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
		uint8 r, g, b, unused;    // unused는 알파가 아님 (항상 0)
	};
	static_assert(sizeof(WPE) == 4, "WPE must be 4 bytes");

#pragma pack(pop)

	struct TileSet
	{
		std::vector<CV5> cv5;
		std::vector<VX4> vx4;
		std::vector<VF4> vf4;
		std::vector<VR4> vr4;
		std::vector<WPE> wpe;
	};

	//스타크래프트 맵 데이터 속 "ERA" 안에 들어있는 정보 
	enum class TileSetType : uint32
	{
		kBadlands = 0x00,
		kSpace_platform = 0x01,
		kInstallation = 0x02,
		kAshworld = 0x03,
		kJungle = 0x04,
		kDesert = 0x05,
		kIce = 0x06,
		kTwilight = 0x07,
		kEND = 0x08
	};

	constexpr std::array<std::string_view, (size_t)TileSetType::kEND> kTileSetTypeNames = {
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

	constexpr uint32 ChunkIDByteStride = 4u;

	// 글자 수가 적을 경우 '띄어쓰기' 포함
	// 길이는 '4' 고정 -> uint32 등으로 캐스팅해서 비교 가능
	constexpr std::string_view SCChunkTypeName[(uint32)SCChunkType::kEND] = {
	"ERA ", "DIM ", "MTXM", "UNIT"
	};

	constexpr uint32 ChunkID(const std::string_view s)
	{
		if (s.size() != 4) throw "4 chars required";

		return (uint32)(uint8)s[0] | ((uint32)(uint8)s[1] << 8)
			| ((uint32)(uint8)s[2] << 16) | ((uint32)(uint8)s[3] << 24);
	}

	constexpr std::string IDToChunkName(const uint32 id)
	{
		std::string ret;
		ret.resize(4);

		ret[0] = (char)(id & 0xFF);
		ret[1] = (char)((id >> 8) & 0xFF);
		ret[2] = (char)((id >> 16) & 0xFF);
		ret[3] = (char)((id >> 24) & 0xFF);

		return ret;	
	}

	// 4글자 char -> uint32로 변환
	constexpr uint32 map_data_type_name_IDs[(uint32)SCChunkType::kEND] = {
		ChunkID(SCChunkTypeName[0]),
		ChunkID(SCChunkTypeName[1]),
		ChunkID(SCChunkTypeName[2]),
		ChunkID(SCChunkTypeName[3])
	};

	struct MapData
	{
		uint32 map_width = 0;
		uint32 map_height = 0;
	};

#pragma pack(push, 1)
	struct UnitData
	{
		uint32 unit_info;
		uint16 pos_x;
		uint16 pos_y;
		uint16 id;          //유닛 ID
		uint16 linked_unit_flag;         //애드온, 나이더스 커널 등 연결된 유닛 유무
		uint16 property;    //유닛 속성(아래 참고 - 아직 사용하지는 않음)

		uint16 upgradable_property;  //유닛의 상태변화에 적용가능한 속성(체력통 증가, 마나통 증가 등등) - 사용 X

		uint8 owner;
		uint8 hp_percent;
		uint8 shield_percent;
		uint8 energy_percent;

		uint32 resources;
		uint16 num_unit_in_container;

		uint16 unit_state_flag;
		uint32 unused;
		uint32 linked_unit_class_inst; //연결된 유닛(애드온, 나이더스 커널)의 인스턴스 정보
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

	using MTXM = uint16;

	struct RGBA { uint8 r, g, b, a; };

	
	constexpr uint32 MTXMGroup(uint16 v) { return (v & 0b0111'1111'1111'0000) >> 4; }
	constexpr uint32 MTXMTile(uint16 v) { return  v & 0b0000'0000'0000'1111; }



	class Texture2D;
	class SCMapLoader
	{
	public:
		SCMapLoader();
		~SCMapLoader();

		bool LoadTilesetData();

		bool LoadMapData(const stdfs::path& map_path);

		s_ptr<Texture2D> GetMapTexture() const { return map_texture_; }

	private:
		StringHashTable<std::vector<uint8>> ParseMapData(const std::vector<uint8>& map_data);

		// return {0, 0} if the map is invalid
		std::array<uint32, 2> GetMegaTileCount(const StringHashTable<std::vector<uint8>>& map_data_table);

		// return TileSetType::kEND if the map is invalid
		TileSetType GetTerrainType(const StringHashTable<std::vector<uint8>>& map_data_table);

		std::vector<UnitData> GetUnitData(const StringHashTable<std::vector<uint8>>& map_data_table);

		std::vector<MTXM> GetMTXM(const StringHashTable<std::vector<uint8>>& map_data_table, uint32 megatile_width, uint32 megatile_height);

		std::vector<TileSet> tileset_data_ = {};

		s_ptr<Texture2D> map_texture_ = {};
	};
}


