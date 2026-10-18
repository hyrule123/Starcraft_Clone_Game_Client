#pragma once
#include <Engine/Core/CoreMinimal.h>
#include <Engine/Core/Math.h>

#include <Engine/Core/Singleton.h>

#include <Content/SCData/SCMapCommon.h>
#include <Content/SCData/SCSpriteCommon.h>
#include <Content/SCData/MPQ/MPQFileSystem.h>
#include <Content/SCData/SCImageLibrary.h>

#include <array>

namespace engine
{
	class Texture2D;
	class Material;
	class PipelineState;
	class Mesh;
	class DepthStencilState;
}

namespace sc
{
	class SCMapBakeComputePass;
	class SCImageLibrary;
	class MPQFileSystem;

	class SCResourceManager
	{
		DECLARE_SINGLETON(SCResourceManager)
	public:
		void Init();
		bool HasInitialized() const { return has_initialized_; }

		const std::filesystem::path& GetMPQDir() const { return mpq_dir_; }

		std::unique_ptr<SCMap> LoadMapDataCPU(const std::filesystem::path& map_path);
		std::unique_ptr<SCMap> LoadMapDataGPU(const std::filesystem::path& map_path);

		const TileSetData* GetTileSet(TileSetType terrain_type) const {
			if ((size_t)terrain_type >= tilesets_.size()) { return nullptr; }
			return &tilesets_[(size_t)terrain_type];
		}

		std::shared_ptr<engine::TypedBuffer> GetWPEColorPaletteBuffer(TileSetType terrain_type) const {
			if ((size_t)terrain_type >= tileset_gpu_buffers_.size()) { return nullptr; }
			return tileset_gpu_buffers_[(size_t)terrain_type].WPE_color_palettes;
		}

		std::shared_ptr<engine::TypedBuffer> GetPCXTilesetLUTBuffer(TileSetType terrain_type) const {
			if ((size_t)terrain_type >= tileset_gpu_buffers_.size()) { return nullptr; }
			return tileset_gpu_buffers_[(size_t)terrain_type].PCX_tileset_LUT;
		}

		const sc::SCImageLibrary& GetSCImageLibrary() const { return sc_image_library_; }

		std::shared_ptr<engine::Material> GetImageMaterial(uint32 image_type, SCImageMaterialVariant variant);
		std::shared_ptr<engine::Mesh> GetImageMesh() const { return image_mesh_; }

		std::shared_ptr<engine::Mesh> GetMapMesh() const { return map_mesh_; }
		std::shared_ptr<engine::Material> GetMapMaterial() const;

	private:
		bool LoadTilesetData(const MPQFileSystem& mpq_file_system);
		bool CreateTileSetGPUData(const std::array<TileSetData, (size_t)TileSetType::kCount>& tileset_data);
		SCMapLoadingData LoadMapLoadingData(const std::filesystem::path& map_path);

		void CreateDepthStencilStates();
		void CreateOriginalMaterials();
		void CreateMesh();
		void CreateMapMaterial();

		engine::StringHashTable<std::vector<uint8>> ParseMapData(const std::vector<uint8>& map_data);

		// return {0, 0} if the map is invalid
		std::array<uint32, 2> GetMegaTileCount(const engine::StringHashTable<std::vector<uint8>>& map_data_table);

		// return TileSetType::kEND if the map is invalid
		TileSetType GetTerrainType(const engine::StringHashTable<std::vector<uint8>>& map_data_table);

		std::vector<UnitData> GetUnitData(const engine::StringHashTable<std::vector<uint8>>& map_data_table);

		std::vector<MTXM> GetMTXM(const engine::StringHashTable<std::vector<uint8>>& map_data_table, uint32 megatile_width, uint32 megatile_height);

	private:
		// 리소스가 저장되어 있는 MPQ 모음
		MPQFileSystem mpq_file_system_ = {};

		// GRP 이미지 라이브러리
		sc::SCImageLibrary sc_image_library_ = {};

		// GRP 별 Material, modifier에 따라 다른 Material을 사용해야 함
		std::vector<std::array<std::shared_ptr<engine::Material>, (size_t)SCImageMaterialVariant::kCount>> materials_ = {};
		std::array<std::shared_ptr<engine::Material>, (size_t)SCImageMaterialVariant::kCount> orig_materials_ = {};
		std::shared_ptr<engine::Mesh> image_mesh_ = {};

		// Index = TileSet Type (Jungle, Desert, Ice, Twilight, etc.)
		std::array<TileSetData, (size_t)TileSetType::kCount> tilesets_ = {};
		std::array<TileSetDataGPU, (size_t)TileSetType::kCount> tileset_gpu_buffers_ = {};

		// GPU
		std::shared_ptr<engine::DepthStencilState> depth_key_write_dss_ = {};
		std::shared_ptr<engine::DepthStencilState> depth_key_test_dss_ = {};


		std::unique_ptr<SCMapBakeComputePass> sc_map_baker_ = {};

		std::shared_ptr<engine::Mesh> map_mesh_ = {};
		std::shared_ptr<engine::Material> map_material_ = {};

		// Common
		std::shared_ptr<engine::TypedBuffer> tunit_buffer_ = {};

		// etc
		std::filesystem::path mpq_dir_;
		bool has_initialized_ = false;
	};
}


