#include "Content/pch.h"
#include "SCMapLoader.h"

#include <Engine/Manager/ResourceManager.h>

#include <Engine/Core/Debug.h>

#include <Engine/Util/file_system.h>

#include <Engine/Resource/GPU/Buffer/Texture2D.h>
#include <Engine/Resource/GPU/Buffer/StructuredBuffer.h>
#include <Engine/Resource/GPU/Buffer/TypedBuffer.h>

#include <Content/SCData/MPQArchive.h>
#include <Content/SCData/SCMapBakeComputePass.h>

#include <fstream>
#include <iterator>
#include <algorithm>


namespace engine
{
	SCMapLoader::SCMapLoader()
	{
		LoadTilesetData();
	}

	SCMapLoader::~SCMapLoader()
	{}
	bool SCMapLoader::LoadTilesetData()
	{
		stdfs::path tileset_path = ResourceManager::GetInst().GetResourceDir();
		tileset_path /= "SCMap";
		tileset_path /= "TileSet";

		for (size_t TileSetIdx = 0; TileSetIdx < (size_t)TileSetType::kEND; ++TileSetIdx)
		{
			tilesets_[TileSetIdx] = TileSet();
			TileSet& tileset = tilesets_[TileSetIdx];

			stdfs::path each_tileset_path = tileset_path / kTileSetTypeNames[TileSetIdx];

			// CV5, VX4, VF4, VR4, WPE 파일을 순차적으로 읽어와서 tileset_data_에 저장
			each_tileset_path.replace_extension(".CV5");
			std::ios::openmode mode = std::ios::binary | std::ios::in;
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
				ERR_MSG("Failed to open Tilemap Data Files.");
				return false;
			}

			tileset.cv5 = file_system::ReadAll<CV5>(fpCV5);
			tileset.vf4 = file_system::ReadAll<VF4>(fpVF4);
			tileset.vr4 = file_system::ReadAll<VR4>(fpVR4);
			tileset.vx4 = file_system::ReadAll<VX4>(fpVX4);
			tileset.wpe = file_system::ReadAll<WPE>(fpWPE);
		}
		
		sc_map_baker_ = EntityManager::CreateEntity<SCMapBakeComputePass>();
		bool result = sc_map_baker_->CreateTileSetGPUData(tilesets_);
		if (!result)
		{
			ERR_MSG("Failed to create TileSet GPU data.");
			return false;
		}

		return true;
	}

	u_ptr<SCMap> SCMapLoader::LoadMapDataCPU(const stdfs::path& map_path)
	{
		ASSERT(sc_map_baker_ != nullptr);

		SCMapLoadingData map_data = LoadMapLoadingData(map_path);

		// 지형 타입에 맞는 타일셋
		TileSet& tileset = tilesets_[(size_t)map_data.terrain_type];

		// 픽셀 색 정보 컨테이너 할당
		size_t pixel_width = (size_t)map_data.megatile_width * (size_t)32;
		size_t pixel_height = (size_t)map_data.megatile_height * (size_t)32;
		std::vector<uint8> WPE_idx;
		WPE_idx.resize(pixel_width * pixel_height);

		//채워넣기
		for (uint32 ty = 0; ty < map_data.megatile_height; ++ty)
		{
			for (uint32 tx = 0; tx < map_data.megatile_width; ++tx)
			{
				// 1. MTXM에서 타일 정보 가져오기
				const uint16 v = map_data.mtxm[ty * map_data.megatile_width + tx];

				// CV5 인덱스
				const uint32 group = MTXMGroup(v);

				// CV5 내 타일 인덱스 (0 ~ 15)
				const uint32 tile = MTXMTile(v);

				// 널 타일 → 검정
				if (group >= tileset.cv5.size()) { continue; }

				// CV5[group]의 CV5_megatiles[tile]
				const uint16 mega = tileset.cv5[group].megatiles[tile];
				if (mega >= tileset.vx4.size()) { continue; }

				// 메가타일 안의 미니타일 4*4 참조
				for (uint32 m = 0; m < 16; ++m)
				{
					// VX4에서 VR4 참조 및 반전 여부 확인
					const uint16 ref = tileset.vx4[mega].minitiles[m];
					const uint16 vr4_id = VX4Ref(ref);
					const bool   flipped = VX4Flipped(ref);
					if (vr4_id >= tileset.vr4.size()) { continue; }

					const uint32 mx = (m % 4) * 8;
					const uint32 my = (m / 4) * 8;

					for (uint32 py = 0; py < 8; ++py)
					{
						for (uint32 px = 0; px < 8; ++px)
						{
							// VR4에서 WPE 색인 가져오기
							const uint8 idx = tileset.vr4[vr4_id].pixels[py][flipped ? 7 - px : px];

							// 최종 픽셀 좌표 계산
							const uint32 x = tx * 32 + mx + px;
							const uint32 y = ty * 32 + my + py;

							WPE_idx[y * pixel_width + x] = idx;

							// 색상을 가져오지 않고, WPE 색인만 저장하도록 변경했음.
							//const WPE& c = tileset->wpe[idx];
							//RGBA& pixel = pixels[y * pixel_width + x];
							//pixel.r = c.r;
							//pixel.g = c.g;
							//pixel.b = c.b;
							//pixel.a = 0xFF;	 // 알파 채널은 항상 255로 설정(원래 없음)
						}
					}

				}
			}
		}

		// Texture 생성
		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width = (UINT)pixel_width;
		desc.Height = (UINT)pixel_height;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R8_UINT;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_IMMUTABLE;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

		D3D11_SUBRESOURCE_DATA init_data = {};
		init_data.pSysMem = WPE_idx.data();
		init_data.SysMemPitch = (uint32)(sizeof(uint8)) * (uint32)pixel_width;      // 바이트 단위

		u_ptr<SCMap> result_map = std::make_unique<SCMap>();
		result_map->texture = EntityManager::CreateEntity<Texture2D>();
		bool result = result_map->texture->CreateTexture2D(&desc, &init_data);
		if (result == false) { return nullptr; }

		result_map->megatile_width = map_data.megatile_width;
		result_map->megatile_height = map_data.megatile_height;
		result_map->terrain_type = map_data.terrain_type;
		result_map->unit_data = std::move(map_data.unit_data);
		result_map->wpe_color_palettes = sc_map_baker_->GetWPEColorPaletteBuffer(map_data.terrain_type);

		return result_map;
	}
	u_ptr<SCMap> SCMapLoader::LoadMapDataGPU(const stdfs::path& map_path)
	{
		SCMapLoadingData map_data = LoadMapLoadingData(map_path);

		u_ptr<SCMap> result_map = std::make_unique<SCMap>();
		result_map->texture = sc_map_baker_->BakeMapTexture(map_data);
		if(result_map->texture == nullptr) { return nullptr; }
		
		result_map->megatile_width = map_data.megatile_width;
		result_map->megatile_height = map_data.megatile_height;
		result_map->terrain_type = map_data.terrain_type;
		result_map->unit_data = std::move(map_data.unit_data);
		result_map->wpe_color_palettes = sc_map_baker_->GetWPEColorPaletteBuffer(map_data.terrain_type);

		return result_map;
	}

	SCMapLoadingData SCMapLoader::LoadMapLoadingData(const stdfs::path& map_path)
	{
		MPQArchive mpq_archive;
		bool result = mpq_archive.OpenMPQFile(map_path);

		if (false == result)
		{
			ERR_MSG("Failed to open MPQ archive.");
			return SCMapLoadingData{};
		}

		SCMapLoadingData map_data = {};

		map_data.map_raw_bytes = mpq_archive.OpenInnerFile("staredit\\scenario.chk");
		if (map_data.map_raw_bytes.empty())
		{
			ERR_MSG("Failed to open scenario.chk file.");
			return SCMapLoadingData{};
		}

		map_data.map_data_table = ParseMapData(map_data.map_raw_bytes);

		// 맵 사이즈
		auto megatile_count = GetMegaTileCount(map_data.map_data_table);
		const uint32& megatile_width = megatile_count[0];
		const uint32& megatile_height = megatile_count[1];
		if (megatile_width == 0 || megatile_height == 0) { return SCMapLoadingData{}; }
		map_data.megatile_width = megatile_width;
		map_data.megatile_height = megatile_height;

		// 지형 타입
		auto terrain_type = GetTerrainType(map_data.map_data_table);
		if (terrain_type == TileSetType::kEND) { return SCMapLoadingData{}; }
		map_data.terrain_type = terrain_type;

		// MTXM 데이터
		map_data.mtxm = GetMTXM(map_data.map_data_table, megatile_width, megatile_height);
		if (map_data.mtxm.empty()) { return SCMapLoadingData{}; }

		// Unit Data (비어있어도 무관)
		map_data.unit_data = GetUnitData(map_data.map_data_table);

		return std::move(map_data);
	}

	StringHashTable<std::vector<uint8>> SCMapLoader::ParseMapData(const std::vector<uint8>& map_data)
	{
		// ERA, DIM, MTXM, UNIT 파일을 순차적으로 읽어와서 처리
		StringHashTable <std::vector<uint8>> map_data_table;

		int64 cursor = 0;
		
		constexpr size_t kMaxIterations = 1000000; // 안전장치: 무한루프 방지
		for(size_t i = 0; i < kMaxIterations; ++i)
		{
			if (cursor + 8 > (int64)map_data.size())
			{
				break;
			}

			// type_name == 4바이트
			std::string_view type_name(reinterpret_cast<const char*>(&map_data[cursor]), ChunkIDByteStride);

			// uint32 형태로 비교 가능하나, 타입명을 확인하기 어려워서 일단은 string_view로 처리
			uint32 chunk_ID = ChunkID(type_name);

			// data length는 음수가 될 수 있다. (커서를 앞으로 보냄)
			int32  data_length;
			memcpy(&data_length, &map_data[cursor + ChunkIDByteStride], sizeof(data_length));
			if (data_length < 0)
			{
				ERR_MSG("Invalid data length. Map may be protected. Please check.");
				break;
			}

			int64 next_cursor = cursor + (int64)ChunkIDByteStride * 2ull + (int64)data_length;

			if (0 <= next_cursor && next_cursor <= (int64)map_data.size())
			{
				auto it = map_data_table.find(type_name);
				if (it != map_data_table.end())
				{
					// 이미 존재하는 type_name이면 에러 메시지 출력 
					// 맵 데이터에 보호가 걸려있는 거라서 처리가 필요함
					ERR_MSG("Already found type_name. Please check");
				}

				// 데이터 길이가 양수일 경우 데이터를 읽어준다
				if (data_length > 0)
				{
					auto start = map_data.begin() + cursor + (int64)ChunkIDByteStride * 2;
					auto end = start + data_length;
					map_data_table[type_name] = std::vector<uint8>(start, end);
				}

				cursor = next_cursor;
			}
			else
			{
				break;
			}
		}

		return map_data_table;
	}
	std::array<uint32, 2> SCMapLoader::GetMegaTileCount(const StringHashTable<std::vector<uint8>>& map_data_table)
	{
		auto it = map_data_table.find(SCChunkTypeName[(uint32)SCChunkType::kMapSize]);
		
		if (it == map_data_table.end())
		{
			ERR_MSG("DIM chunk not found in map data.");
			return {};
		}

		const std::vector<uint8>& dim_data = it->second;
		if (dim_data.size() < 4)
		{
			ERR_MSG("DIM chunk data must exceed 4 bytes.");
			return {};
		}

		uint16 width = 0;
		uint16 height = 0;

		memcpy(&width, dim_data.data(), sizeof(uint16));
		memcpy(&height, dim_data.data() + sizeof(uint16), sizeof(uint16));

		if (!(1 <= width && width <= 256 && 1 <= height && height <= 256))
		{
			ERR_MSG("Invalid map size. Width and height must be between 1 and 256.");
			return {};
		}

		return {(uint32)width, (uint32)height};
	}

	// return TileSetType::kEND if the map is invalid
	TileSetType SCMapLoader::GetTerrainType(const StringHashTable<std::vector<uint8>>& map_data_table)
	{
		static_assert((int32)TileSetType::kEND == 8, "TileSetType::kEND must be 8.");

		auto it = map_data_table.find(SCChunkTypeName[(uint32)SCChunkType::kTerrain]);

		if (it == map_data_table.end())
		{
			ERR_MSG("ERA chunk not found in map data.");
			return TileSetType::kEND;
		}

		const std::vector<uint8>& era_data = it->second;
		if(era_data.size() < 2)
		{
			ERR_MSG("ERA chunk data must at least 2 bytes.");
			return TileSetType::kEND;
		}

		//첫 1바이트에 지형 정보가 들어있다.
		uint8 terrain_byte = era_data[0];

		//앞의 5비트는 0으로 변환 (유효 비트는 하위 3비트: 0~7)
		constexpr uint8 terrain_mask = 0b00000111;
		terrain_byte &= terrain_mask;

		return static_cast<TileSetType>(terrain_byte);
	}

	std::vector<UnitData> SCMapLoader::GetUnitData(const StringHashTable<std::vector<uint8>>& map_data_table)
	{
		auto it = map_data_table.find(SCChunkTypeName[(uint32)SCChunkType::kUnitPlacement]);

		if (it == map_data_table.end())
		{
			ERR_MSG("UNIT chunk not found in map data.");
			return std::vector<UnitData>();
		}

		const std::vector<uint8>& unit_data_chunk = it->second;

		// UnitData 구조체는 36바이트 단위
		size_t num_data = unit_data_chunk.size() / sizeof(UnitData);

		// 저장된 데이터를 옮긴다.
		std::vector<UnitData> unit_data_vector;
		if (num_data > 0)
		{
			unit_data_vector.resize(num_data);
			memcpy(unit_data_vector.data(), unit_data_chunk.data(), sizeof(UnitData) * num_data);
		}

		return unit_data_vector;
	}
	std::vector<MTXM> SCMapLoader::GetMTXM(const StringHashTable<std::vector<uint8>>& map_data_table, uint32 megatile_width, uint32 megatile_height)
	{
		auto it = map_data_table.find(SCChunkTypeName[(uint32)SCChunkType::kTileMapAtlas]);
		if (it == map_data_table.end())
		{
			ERR_MSG("MTXM chunk not found in map data.");
			return {};
		}

		const std::vector<uint8>& mtxm_data = it->second;
		
		std::vector<MTXM> mtxms((size_t)megatile_width * (size_t)megatile_height);

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
		// 데이터 사이즈가 모자라도 정상 동작이라고 함(null tile)
		size_t copy_size = std::min(mtxm_data.size(), mtxms.size() * sizeof(MTXM));

		memcpy(mtxms.data(), mtxm_data.data(), copy_size);

		return mtxms;
	}
}

