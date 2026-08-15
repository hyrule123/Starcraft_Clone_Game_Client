#pragma once
#include <Engine/Core/Entity.h>

#include <Engine/Core/CoreMinimal.h>
#include <Engine/Core/Math.h>

namespace engine
{
	//단위크기 : 2byte
//총 갯수 : 맵의 가로크기(메가타일) * 세로크기(메가타일)
//HLSL에서는 8개씩 패킹해서 전달(4바이트 int 4개)
#define MXTM_PACK 8u
//MXTM_MAX는 매번 달라지므로 주의(맵사이즈)
	struct MXTM
	{
		uint32 MXTM_Pack[4];
	};

	//단위크기 : 20 byte(dummy) + 2 byte * 16 = 52 byte
	//총 갯수 : 4096개
	//HLSL에서는 더미 데이터는 버리고 메가타일 정보가 들어있는 나머지 정보만 추출해서 들여올 예정
	//2byte * 16 = 32 byte
#define CV5_MAX 2048u
	struct CV5
	{
		UINT32 TerrainType;
		UINT32 Flags;
		float2 Padding;



		//Megatile
		UINT32 MegaTileIndex[8];
	};


	//단위크기: 2 byte(비트 플래그) * 16(미니타일의 갯수) = 32byte, 통과가능여부, 높이
	//총 갯수 : 65536개
	//HLSL에서는 4byte * 8개로 패킹해서 가져올 예정 -> 갯수 = 65536
#define VF4_PACK 16u
#define VF4_MAX 65536u
	struct VF4
	{
		//32 byte length
		UINT32 MiniTileFlags[8];
	};


	//단위크기: 2 byte * 16 = 32byte, megatile이 어떤 minitile로 이루어졌는지 표시
	//총 갯수 : 65536개
#define VX4_MAX 65536u
	struct VX4
	{
		//VR4에서의 인덱스를 저장함(변수명 틀린거 아님)
		UINT32 VR4Index[8];
	};

	//단위 크기: 1 byte * 64 = 64byte - mini tile각각의 픽셀이 WPE 팔레트의 몇 번 색에 해당하는지를 저장(0 ~ 255 사이의 인덱스가 저장되어있음)
	//미니타일은 8 * 8 픽셀임. 그러니까 하나의 미니타일에서 각 타일이 WPE에서 가져와야 하는 색상의 인덱스를 0 ~ 255 사이로 저장하는 것.
	//32768개
#define VR4_MAX 32768u
#define VR4_PACK 4u
	struct VR4
	{
		UINT32 ColorPack[16];
	};

	//WPE Data : 단위크기 1 byte * 4 = 4 byte (RGBA) - A는 없음(단순 패딩).
	//256개
	//4Byte만 보낼수는 없으므로 4개 묶음을 하나로 묶어서 보내줘야 한다. -> 256 / 4 = 64
#define WPE_PACK 4u
#define WPE_MAX 64u
	struct WPE
	{
		UINT32 RGBAPack[4];
	};

	struct TileSet
	{
		CV5 cv5[CV5_MAX];
		VX4 vx4[VX4_MAX];
		VF4 vf4[VF4_MAX];
		VR4 vr4[VR4_MAX];
		WPE wpe[WPE_MAX];
	};

	//스타크래프트 맵 데이터 속 "ERA" 안에 들어있는 정보 
	enum class TileSetType : UINT32
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

	class SCMapLoader :
		public Entity
	{
		ENTITY_INFO(SCMapLoader, Entity)
	public:
		SCMapLoader();
		virtual ~SCMapLoader() override;

		bool LoadTilesetData();

	private:
		u_ptr<TileSet[]> tileset_data_ = {};
	};
}


