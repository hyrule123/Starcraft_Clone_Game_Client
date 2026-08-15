#include "Content/pch.h"
#include "SCMapLoader.h"

#include <Engine/Manager/ResourceManager.h>

#include <Engine/Core/Debug.h>

#include <fstream>

namespace engine
{
	SCMapLoader::SCMapLoader()
		: Entity(SCMapLoader::kClassConcreteName)
	{}

	SCMapLoader::~SCMapLoader()
	{}
	bool SCMapLoader::LoadTilesetData()
	{
		stdfs::path tileset_path = ResourceManager::GetInst().GetResourceDir();
		tileset_path /= "SCMap";
		tileset_path /= "TileSet";

		// kEND까지의 TileSet을 담을 수 있는 배열을 생성
		tileset_data_ = std::make_unique<TileSet[]>((size_t)TileSetType::kEND);
		memset(tileset_data_.get(), 0, sizeof(TileSet) * (size_t)TileSetType::kEND);

		for (int TileSetIdx = 0; TileSetIdx < (int)TileSetType::kEND; ++TileSetIdx)
		{
			TileSet& tileset = tileset_data_[TileSetIdx];

			stdfs::path each_tileset_path = tileset_path / kTileSetTypeNames[TileSetIdx];

			// CV5, VX4, VF4, VR4, WPE 파일을 순차적으로 읽어와서 tileset_data_에 저장
			each_tileset_path += ".CV5";
			std::ios::openmode mode = std::ios::beg | std::ios::binary | std::ios::in;
			std::ifstream fpCV5(each_tileset_path, mode);

			each_tileset_path.replace_extension(".VX4");
			std::ifstream fpVX4(each_tileset_path, mode);

			each_tileset_path.replace_extension(".VR4");
			std::ifstream fpVR4(each_tileset_path, mode);

			each_tileset_path.replace_extension(".WPE");
			std::ifstream fpWPE(each_tileset_path, mode);

			each_tileset_path.replace_extension(".VF4");
			std::ifstream fpVF4(each_tileset_path, mode);

			if (
				false == fpCV5.is_open() ||
				false == fpVX4.is_open() ||
				false == fpVR4.is_open() ||
				false == fpWPE.is_open() ||
				false == fpVF4.is_open()
				)
			{
				ERROR_MESSAGE("Failed to open Tilemap Data Files.");
				return false;
			}

			UINT16 cpy = (UINT16)0;
			for (uint32 i = 0; i < CV5_MAX; ++i)
			{
				//각각 2바이트씩 읽어와서 캐스트해서 구조체에 넣어준다.
				fpCV5.read((char*)&cpy, sizeof(UINT16));
				tileset.cv5[i].TerrainType = (UINT32)cpy;
				cpy = 0;

				fpCV5.read((char*)&cpy, sizeof(UINT16));
				tileset.cv5[i].Flags = (UINT32)cpy;
				cpy = 0;

				//커서를 16만큼 앞으로 이동한뒤
				fpCV5.seekg(16, std::ios::cur);

				//다시 32바이트를 읽는다.
				fpCV5.read((char*)&(tileset.cv5[i].MegaTileIndex), sizeof(UINT16) * (size_t)16);

				if (true == fpCV5.eof()) { break; }
			}

			fpVX4.read((char*)&(tileset.vx4), sizeof(VX4) * VX4_MAX);
			fpVR4.read((char*)&(tileset.vr4), sizeof(VR4) * VR4_MAX);
			fpWPE.read((char*)&(tileset.wpe), sizeof(WPE) * WPE_MAX);
			fpVF4.read((char*)&(tileset.vf4), sizeof(VF4) * VF4_MAX);
		}

		return true;
	}
}

