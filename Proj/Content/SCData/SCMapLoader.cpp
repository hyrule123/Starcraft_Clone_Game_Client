#include "Content/pch.h"
#include "SCMapLoader.h"

#include <Engine/Core/Debug.h>

#include <Engine/Resource/GPU/Buffer/Texture2D.h>
#include <Engine/Resource/GPU/Buffer/TypedBuffer.h>

#include <Content/SCData/MPQ/MPQArchive.h>
#include <Content/SCData/MPQ/MPQFileSystem.h>
#include <Content/SCData/SCMapBakeComputePass.h>
#include <Content/SCData/SCTileSetPcxLUT.h>

#include <algorithm>
#include <span>

namespace sc
{
	namespace
	{
		using ChunkTable = engine::StringHashTable<std::vector<uint8>>;

		// .chk 청크를 순차 순회하며 [이름 4][길이 4][데이터] 단위로 테이블에 저장. 같은 청크는 뒤가 이김
		ChunkTable ParseMapData(const std::vector<uint8>& map_data)
		{
			ChunkTable map_data_table;

			int64 cursor = 0;

			constexpr size_t kMaxIterations = 1000000; // 안전장치: 무한루프 방지
			for (size_t i = 0; i < kMaxIterations; ++i)
			{
				if (cursor + 8 > (int64)map_data.size())
				{
					break;
				}

				// type_name == 4바이트
				std::string_view type_name(reinterpret_cast<const char*>(&map_data[cursor]), ChunkIDByteStride);

				// data length는 음수가 될 수 있다. (커서를 앞으로 보냄)
				int32 data_length;
				memcpy(&data_length, &map_data[cursor + ChunkIDByteStride], sizeof(data_length));
				if (data_length < 0)
				{
					ERR_MSG("Invalid data length. Map may be protected. Please check.");
					break;
				}

				const int64 next_cursor = cursor + (int64)ChunkIDByteStride * 2ll + (int64)data_length;

				if (0 <= next_cursor && next_cursor <= (int64)map_data.size())
				{
					if (map_data_table.find(type_name) != map_data_table.end())
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

		// DIM. 실패 시 {0, 0}
		std::array<uint32, 2> GetMegaTileCount(const ChunkTable& map_data_table)
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

			return { (uint32)width, (uint32)height };
		}

		// ERA. 실패 시 TileSetType::kCount
		TileSetType GetTerrainType(const ChunkTable& map_data_table)
		{
			static_assert((int32)TileSetType::kCount == 8, "TileSetType::kCount must be 8.");

			auto it = map_data_table.find(SCChunkTypeName[(uint32)SCChunkType::kTerrain]);
			if (it == map_data_table.end())
			{
				ERR_MSG("ERA chunk not found in map data.");
				return TileSetType::kCount;
			}

			const std::vector<uint8>& era_data = it->second;
			if (era_data.size() < 2)
			{
				ERR_MSG("ERA chunk data must at least 2 bytes.");
				return TileSetType::kCount;
			}

			// 유효 비트는 하위 3비트 (0~7)
			constexpr uint8 kTerrainMask = 0b00000111;
			return static_cast<TileSetType>(era_data[0] & kTerrainMask);
		}

		// UNIT. 청크가 없으면 빈 vector (맵에 배치 유닛이 없어도 정상)
		std::vector<UnitData> GetUnitData(const ChunkTable& map_data_table)
		{
			auto it = map_data_table.find(SCChunkTypeName[(uint32)SCChunkType::kUnitPlacement]);
			if (it == map_data_table.end())
			{
				ERR_MSG("UNIT chunk not found in map data.");
				return {};
			}

			const std::vector<uint8>& unit_data_chunk = it->second;

			// UnitData 구조체는 36바이트 단위
			const size_t num_data = unit_data_chunk.size() / sizeof(UnitData);

			std::vector<UnitData> unit_data_vector;
			if (num_data > 0)
			{
				unit_data_vector.resize(num_data);
				memcpy(unit_data_vector.data(), unit_data_chunk.data(), sizeof(UnitData) * num_data);
			}

			return unit_data_vector;
		}

		// MTXM. DIM 크기로 버퍼를 잡고 청크 길이만큼만 복사 (모자란 부분은 null tile). 청크가 없으면 빈 vector
		std::vector<MTXM> GetMTXM(const ChunkTable& map_data_table, uint32 megatile_width, uint32 megatile_height)
		{
			auto it = map_data_table.find(SCChunkTypeName[(uint32)SCChunkType::kTileMapAtlas]);
			if (it == map_data_table.end())
			{
				ERR_MSG("MTXM chunk not found in map data.");
				return {};
			}

			const std::vector<uint8>& mtxm_data = it->second;

			std::vector<MTXM> mtxms((size_t)megatile_width * (size_t)megatile_height);

			const size_t copy_size = (std::min)(mtxm_data.size(), mtxms.size() * sizeof(MTXM));
			memcpy(mtxms.data(), mtxm_data.data(), copy_size);

			return mtxms;
		}
	} // namespace

	SCMapLoader::SCMapLoader() = default;
	SCMapLoader::~SCMapLoader() = default;

	void SCMapLoader::Init(const MPQFileSystem& mpq_file_system)
	{
		using namespace engine;

		CHECK_F(has_initialized_ == false, "Double initialization");
		has_initialized_ = true;

		bool result = LoadTilesetData(mpq_file_system);
		CHECK_F(result, "Failed to load TileSet data");

		result = CreateTileSetGPUData();
		CHECK_F(result, "Failed to create TileSet GPU data");

		sc_map_baker_ = EntityManager::CreateEntity<SCMapBakeComputePass>();
	}

	std::unique_ptr<SCMap> SCMapLoader::LoadMapGPU(const std::filesystem::path& map_path)
	{
		ASSERT(sc_map_baker_ != nullptr);

		std::optional<SCMapLoadingData> map_data = LoadMapLoadingData(map_path);
		if (false == map_data.has_value())
		{
			return nullptr;
		}

		std::unique_ptr<SCMap> result_map = std::make_unique<SCMap>();
		result_map->texture = sc_map_baker_->BakeMapTexture(*map_data);
		if (result_map->texture == nullptr)
		{
			return nullptr;
		}

		FillCommonMapFields(*map_data, *result_map);

		return result_map;
	}

	std::unique_ptr<SCMap> SCMapLoader::LoadMapCPU(const std::filesystem::path& map_path)
	{
		using namespace engine;

		std::optional<SCMapLoadingData> map_data_opt = LoadMapLoadingData(map_path);
		if (false == map_data_opt.has_value())
		{
			return nullptr;
		}
		SCMapLoadingData& map_data = *map_data_opt;

		// 지형 타입에 맞는 타일셋 (LoadMapLoadingData가 범위를 보장)
		const TileSetData& tileset = tilesets_[(size_t)map_data.terrain_type];

		// 픽셀 색 정보 컨테이너 할당
		const size_t pixel_width = (size_t)map_data.megatile_width * (size_t)32;
		const size_t pixel_height = (size_t)map_data.megatile_height * (size_t)32;
		std::vector<uint8> WPE_idx(pixel_width * pixel_height);

		// 채워넣기
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

		std::unique_ptr<SCMap> result_map = std::make_unique<SCMap>();
		result_map->texture = EntityManager::CreateEntity<Texture2D>();
		const bool result = result_map->texture->CreateTexture2D(&desc, &init_data);
		if (result == false)
		{
			return nullptr;
		}

		FillCommonMapFields(map_data, *result_map);

		return result_map;
	}

	bool SCMapLoader::LoadTilesetData(const MPQFileSystem& mpq_file_system)
	{
		const std::filesystem::path tileset_dir = "tileset";

		// 파일 하나를 읽어 대상 vector로 이동. 실패 시 false (원인은 ReadFile이 이미 알림)
		auto read_into = [&mpq_file_system]<typename T>(std::filesystem::path & path, std::string_view extension, std::vector<T>&out) -> bool
		{
			path.replace_extension(extension);
			return mpq_file_system.ReadFile(path, out);
		};

		for (size_t tileset_idx = 0; tileset_idx < static_cast<size_t>(TileSetType::kCount); ++tileset_idx)
		{
			tilesets_[tileset_idx] = {};
			TileSetData& tileset = tilesets_[tileset_idx];
			const std::string_view tileset_name = kTileSetTypeNames[tileset_idx];

			std::filesystem::path each_tileset_path = tileset_dir / tileset_name;

			// CV5, VX4, VR4, WPE, VF4 순서로 읽음. || 는 첫 실패에서 뒤를 평가하지 않음
			if (!read_into(each_tileset_path, ".CV5", tileset.cv5) ||
				!read_into(each_tileset_path, ".VX4", tileset.vx4) ||
				!read_into(each_tileset_path, ".VR4", tileset.vr4) ||
				!read_into(each_tileset_path, ".WPE", tileset.wpe) ||
				!read_into(each_tileset_path, ".VF4", tileset.vf4))
			{
				ERR_MSG("TileSet: 파일 로드 실패 [{}]", tileset_name);
				return false;
			}

			// 개수 관계 검증 (배수 검사만으로는 잡히지 않는 형식 오류)
			if (tileset.wpe.size() != 256)
			{
				ERR_MSG("TileSet [{}]: WPE 색 수가 256이 아님 ({})", tileset_name, tileset.wpe.size());
				return false;
			}
			if (tileset.vf4.size() != tileset.vx4.size())
			{
				ERR_MSG("TileSet [{}]: VF4({})와 VX4({}) 개수 불일치", tileset_name, tileset.vf4.size(), tileset.vx4.size());
				return false;
			}

			tileset.pcx_lut = std::make_unique<sc::SCTileSetPcxLUT>();
			if (!tileset.pcx_lut->Init(mpq_file_system, tileset_name))
			{
				ERR_MSG("TileSet: LUT 초기화 실패 [{}]", tileset_name);
				return false;
			}
		}

		return true;
	}

	bool SCMapLoader::CreateTileSetGPUData()
	{
		using namespace engine;

		auto clear_tilesets = [&]()
			{
				for (TileSetDataGPU& gpu : tileset_gpu_buffers_)
				{
					gpu = TileSetDataGPU();
				}
			};

		std::vector<uint16> megatile_data;
		bool result = false;

		for (size_t tileset_i = 0; tileset_i < tileset_gpu_buffers_.size(); ++tileset_i)
		{
			const TileSetData& tileset = tilesets_[tileset_i];
			tileset_gpu_buffers_[tileset_i] = TileSetDataGPU();
			TileSetDataGPU& tileset_gpu = tileset_gpu_buffers_[tileset_i];

#pragma region CV5 - Megatile
			// CV5 구조체의 CV5_megatiles 배열 크기와 오프셋을 컴파일 타임에 검증
			static_assert(CV5{}.megatiles.size() == megatiles_per_group);
			static_assert(offsetof(CV5, megatiles) == 20);
			if (tileset.cv5.empty())
			{
				ERR_MSG("CV5 data is empty for current tileset index");
				clear_tilesets();
				return false;
			}
			megatile_data.resize(tileset.cv5.size() * megatiles_per_group);

			// CV5에서 megatile index만 추출하여 LUT 생성
			for (size_t j = 0; j < tileset.cv5.size(); ++j)
			{
				std::ranges::copy(tileset.cv5[j].megatiles, megatile_data.begin() + j * megatiles_per_group);
			}

			tileset_gpu.CV5_megatiles = EntityManager::CreateEntity<TypedBuffer>();
			std::span<const uint16> megatile_span = megatile_data;
			result = tileset_gpu.CV5_megatiles->CreateImmutableBuffer(DXGI_FORMAT_R16_UINT, megatile_span);
			if (!result) { clear_tilesets(); return false; }
#pragma endregion CV5 - Megatile

#pragma region VX4 - Minitile
			static_assert(sizeof(VX4) == sizeof(uint16) * minitiles_per_VX4);
			if (tileset.vx4.empty())
			{
				ERR_MSG("VX4 data is empty for tileset index");
				clear_tilesets();
				return false;
			}
			tileset_gpu.VX4_minitiles = EntityManager::CreateEntity<TypedBuffer>();
			result = tileset_gpu.VX4_minitiles->CreateImmutableBuffer(DXGI_FORMAT_R16_UINT, sizeof(VX4::Elem), tileset.vx4.data(), (uint32)(tileset.vx4.size() * minitiles_per_VX4));
			if (!result) { clear_tilesets(); return false; }
#pragma endregion VX4 - Minitile

#pragma region VR4 - Minitile WPE index
			if (tileset.vr4.empty())
			{
				ERR_MSG("VR4 data is empty for current tileset index");
				clear_tilesets();
				return false;
			}
			tileset_gpu.VR4_minitile_wpe_indices = EntityManager::CreateEntity<TypedBuffer>();
			result = tileset_gpu.VR4_minitile_wpe_indices->CreateImmutableBuffer(DXGI_FORMAT_R8_UINT, sizeof(VR4::Elem), tileset.vr4.data(), (uint32)(tileset.vr4.size() * minile_pixel_count));
			if (!result) { clear_tilesets(); return false; }
#pragma endregion VR4 - Minitile WPE index

#pragma region WPE - Color Palette
			if (tileset.wpe.size() != 256)
			{
				ERR_MSG("WPE data is empty or invalid for current tileset index");
				clear_tilesets();
				return false;
			}
			tileset_gpu.WPE_color_palettes = EntityManager::CreateEntity<TypedBuffer>();
			std::span<const WPE> wpe_span = tileset.wpe;
			result = tileset_gpu.WPE_color_palettes->CreateImmutableBuffer(DXGI_FORMAT_R8G8B8A8_UNORM, wpe_span);
			if (!result) { clear_tilesets(); return false; }
#pragma endregion WPE - Color Palette

#pragma region PCX - Tileset LUT
			{
				ASSERT(tileset.pcx_lut);
				std::shared_ptr<TypedBuffer>& pcx_lut_gpu = tileset_gpu.PCX_tileset_LUT;
				pcx_lut_gpu = EntityManager::CreateEntity<TypedBuffer>();

				std::span<const SCTileSetPcxLUT::ElemType> pcx_lut_span = tileset.pcx_lut->data();
				result = pcx_lut_gpu->CreateImmutableBuffer(DXGI_FORMAT_R8_UINT, pcx_lut_span);
				if (!result) { clear_tilesets(); return false; }
			}
#pragma endregion PCX - Tileset LUT
		}

		return true;
	}

	std::optional<SCMapLoadingData> SCMapLoader::LoadMapLoadingData(const std::filesystem::path& map_path)
	{
		MPQArchive mpq_archive;
		if (false == mpq_archive.OpenMPQFile(map_path))
		{
			ERR_MSG("Failed to open MPQ archive.");
			return std::nullopt;
		}

		SCMapLoadingData map_data = {};

		MPQFile map_file = mpq_archive.OpenInnerFile("staredit\\scenario.chk");
		if (map_file.ReadAll(map_data.map_raw_bytes) == false)
		{
			ERR_MSG("Failed to read scenario.chk file.");
			return std::nullopt;
		}

		map_data.map_data_table = ParseMapData(map_data.map_raw_bytes);

		// 맵 사이즈
		const std::array<uint32, 2> megatile_count = GetMegaTileCount(map_data.map_data_table);
		if (megatile_count[0] == 0 || megatile_count[1] == 0)
		{
			return std::nullopt;
		}
		map_data.megatile_width = megatile_count[0];
		map_data.megatile_height = megatile_count[1];

		// 지형 타입
		const TileSetType terrain_type = GetTerrainType(map_data.map_data_table);
		if (terrain_type == TileSetType::kCount)
		{
			return std::nullopt;
		}
		map_data.terrain_type = terrain_type;

		// MTXM 데이터
		map_data.mtxm = GetMTXM(map_data.map_data_table, map_data.megatile_width, map_data.megatile_height);
		if (map_data.mtxm.empty())
		{
			return std::nullopt;
		}

		// Unit Data (비어있어도 무관)
		map_data.unit_data = GetUnitData(map_data.map_data_table);

		map_data.tileset = &tilesets_[(size_t)terrain_type];
		map_data.tileset_gpu = &tileset_gpu_buffers_[(size_t)terrain_type];

		return map_data;
	}

	void SCMapLoader::FillCommonMapFields(SCMapLoadingData& map_data, SCMap& out_map) const
	{
		ASSERT(map_data.terrain_type < TileSetType::kCount);

		const TileSetDataGPU& tileset_gpu = tileset_gpu_buffers_[(size_t)map_data.terrain_type];

		out_map.megatile_width = map_data.megatile_width;
		out_map.megatile_height = map_data.megatile_height;
		out_map.terrain_type = map_data.terrain_type;
		out_map.unit_data = std::move(map_data.unit_data);
		out_map.wpe_color_palettes = tileset_gpu.WPE_color_palettes;
		out_map.pcx_tileset_LUT = tileset_gpu.PCX_tileset_LUT;
	}
}