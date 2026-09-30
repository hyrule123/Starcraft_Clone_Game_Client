#pragma once
#include <Engine/Core/CoreMinimal.h>
#include <Engine/Core/Math.h>

#include <Engine/Core/Singleton.h>

#include <Content/SCData/SCMapCommon.h>

#include <array>

namespace engine
{
	class Texture2D;
}

namespace sc
{
	class SCMapBakeComputePass;
	class MPQFileSystem;

	class SCMapLoader
	{
		DECLARE_SINGLETON(SCMapLoader)
	public:
		bool LoadTilesetData(const MPQFileSystem& mpq_file_system);

		std::unique_ptr<SCMap> LoadMapDataCPU(const std::filesystem::path& map_path);
		std::unique_ptr<SCMap> LoadMapDataGPU(const std::filesystem::path& map_path);

		const TileSet* GetTileSet(TileSetType terrain_type) const {
			if ((size_t)terrain_type >= tilesets_.size()) { return nullptr; }
			return &tilesets_[(size_t)terrain_type];
		}

	private:
		SCMapLoadingData LoadMapLoadingData(const std::filesystem::path& map_path);

		engine::StringHashTable<std::vector<uint8>> ParseMapData(const std::vector<uint8>& map_data);

		// return {0, 0} if the map is invalid
		std::array<uint32, 2> GetMegaTileCount(const engine::StringHashTable<std::vector<uint8>>& map_data_table);

		// return TileSetType::kEND if the map is invalid
		TileSetType GetTerrainType(const engine::StringHashTable<std::vector<uint8>>& map_data_table);

		std::vector<UnitData> GetUnitData(const engine::StringHashTable<std::vector<uint8>>& map_data_table);

		std::vector<MTXM> GetMTXM(const engine::StringHashTable<std::vector<uint8>>& map_data_table, uint32 megatile_width, uint32 megatile_height);

		// Index = TileSet Type (Jungle, Desert, Ice, Twilight, etc.)
		std::array<TileSet, (size_t)TileSetType::kEND> tilesets_ = {};

		// GPU
		std::unique_ptr<SCMapBakeComputePass> sc_map_baker_ = {};

		// Common
		std::shared_ptr<engine::TypedBuffer> tunit_buffer_ = {};
		
	};
}


