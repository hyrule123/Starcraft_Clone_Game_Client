#pragma once
#include <Engine/Core/CoreMinimal.h>
#include <Engine/Core/Math.h>

#include <Content/SCData/SCMapCommon.h>

#include <array>

namespace engine
{


	class Texture2D;
	class SCMapBakeComputePass;
	class SCMapLoader
	{
	public:
		SCMapLoader();
		~SCMapLoader();

		bool LoadTilesetData();

		bool LoadMapDataCPU(const stdfs::path& map_path);
		bool LoadMapDataGPU(const stdfs::path& map_path);

		s_ptr<Texture2D> GetMapTexture() const { return map_texture_; }

	private:
		StringHashTable<std::vector<uint8>> ParseMapData(const std::vector<uint8>& map_data);

		// return {0, 0} if the map is invalid
		std::array<uint32, 2> GetMegaTileCount(const StringHashTable<std::vector<uint8>>& map_data_table);

		// return TileSetType::kEND if the map is invalid
		TileSetType GetTerrainType(const StringHashTable<std::vector<uint8>>& map_data_table);

		std::vector<UnitData> GetUnitData(const StringHashTable<std::vector<uint8>>& map_data_table);

		std::vector<MTXM> GetMTXM(const StringHashTable<std::vector<uint8>>& map_data_table, uint32 megatile_width, uint32 megatile_height);

		// Index = TileSet Type (Jungle, Desert, Ice, Twilight, etc.)
		std::vector<TileSet> tileset_data_ = {};

		// GPU
		u_ptr<SCMapBakeComputePass> sc_map_baker_ = {};

		s_ptr<Texture2D> map_texture_ = {};
	};
}


