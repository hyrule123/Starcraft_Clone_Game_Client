#include "Content/pch.h"
#include "SCMapLoader.h"

#include <algorithm>
#include <span>
#include <iterator>
#include <random>
#include <chrono>

#include <SCSimulator/Include/SCSimulatorTypes.h>

#include <Content/SCData/MPQ/MPQArchive.h>
#include <Content/SCData/MPQ/MPQFileSystem.h>
#include <Content/SCData/SCMapBakeComputePass.h>
#include <Content/SCData/SCTileSetPcxLUT.h>
#include <Content/SCData/SCImageLibrary.h>

#include <Content/HLSL/SCMap/SCMapRender.hlsli>

#include <Engine/Core/Debug.h>

#include <Engine/Resource/GPU/Buffer/Texture2D.h>
#include <Engine/Resource/GPU/Buffer/Texture2DArray.h>
#include <Engine/Resource/GPU/Buffer/TypedBuffer.h>


/*
* Creep Texture(GRP) 확인 결과: 메가타일 사이즈를 넘어가는 프레임은 없었음(넘침 걱정 필요 없음)
[LOG] Creep GRP [badlands]: canvas 32x32, frames 37
[LOG] Creep GRP [badlands]: overflow frames = 0

[LOG] Creep GRP [platform]: canvas 32x32, frames 37
[LOG] Creep GRP [platform]: overflow frames = 0

[LOG] Creep GRP [install]: canvas 32x32, frames 37
[LOG] Creep GRP [install]: overflow frames = 0

[LOG] Creep GRP [ashworld]: canvas 32x32, frames 37
[LOG] Creep GRP [ashworld]: overflow frames = 0

[LOG] Creep GRP [jungle]: canvas 32x32, frames 37
[LOG] Creep GRP [jungle]: overflow frames = 0

[LOG] Creep GRP [Desert]: canvas 32x32, frames 37
[LOG] Creep GRP [Desert]: overflow frames = 0

[LOG] Creep GRP [Ice]: canvas 32x32, frames 37
[LOG] Creep GRP [Ice]: overflow frames = 0

[LOG] Creep GRP [Twilight]: canvas 32x32, frames 37
[LOG] Creep GRP [Twilight]: overflow frames = 0
*/

/*
* Creep 바닥 타일(CV5 그룹 1) 확인 결과: 0~12(kCreepTileCount)가 유효, 13~15는 미사용(0)
* 예외: install(tileset 2)은 그룹 1 전체가 0 → Creep 바닥 타일 없음
*   OpenBW는 검사 없이 cv5.at(1).mega_tile_index[랜덤 0~12]를 그대로 사용 → install에서는 메가타일 0을 그림
*   (원작의 install Creep 표현은 미확인. 실제 맵에서 확인 후 처리 방식 결정)
[LOG] Creep CV5 [tileset 0] group 1: 123, 124, 125, 126, 128, 129, 130, 131, 127, 132, 133, 134, 135, 0, 0, 0
[LOG] Creep CV5 [tileset 1] group 1: 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 0, 0, 0
[LOG] Creep CV5 [tileset 2] group 1: 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
[LOG] Creep CV5 [tileset 3] group 1: 1, 2, 3, 4, 5, 6, 7, 8, 9, 2643, 2644, 2645, 2646, 0, 0, 0
[LOG] Creep CV5 [tileset 4] group 1: 129, 130, 131, 132, 134, 135, 136, 137, 133, 138, 139, 140, 141, 0, 0, 0
[LOG] Creep CV5 [tileset 5] group 1: 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 0, 0, 0
[LOG] Creep CV5 [tileset 6] group 1: 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 0, 0, 0
[LOG] Creep CV5 [tileset 7] group 1: 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 0, 0, 0
*/

namespace sc
{
	// TileSet Offset 범위 검증
	static_assert(sizeof(SCTileSetPcxLUT::TableRange) == sizeof(uint32_t) * 2);
	static_assert(offsetof(SCTileSetPcxLUT::TableRange, offset) == 0);                 // 셰이더 .x
	static_assert(offsetof(SCTileSetPcxLUT::TableRange, rows) == sizeof(uint32_t));    // 셰이더 .y
	static_assert(std::is_trivially_copyable_v<SCTileSetPcxLUT::TableRange>);

	static_assert(kMegatileFlagHasCreep == SCSim::MegatileFlags::HasCreep);

	namespace
	{
		// OpenBW ui.h cloak_fade_selector: WPE 색별 밝기 등급(0~8)
		constexpr uint32_t kCloakFadeSelectorWeightR = 28u;
		constexpr uint32_t kCloakFadeSelectorWeightG = 77u;
		constexpr uint32_t kCloakFadeSelectorWeightB = 151u;	// R + G + B = 256
		constexpr uint32_t kCloakFadeSelectorRound = 4096u;
		constexpr uint32_t kCloakFadeSelectorDivisor = 8192u;

		using ChunkTable = engine::StringHashTable<std::vector<uint8_t>>;

		// .chk 청크를 순차 순회하며 [이름 4][길이 4][데이터] 단위로 테이블에 저장. 같은 청크는 뒤가 이김
		ChunkTable ParseMapData(const std::vector<uint8_t>& map_data)
		{
			ChunkTable map_data_table;

			int64_t cursor = 0;

			constexpr size_t kMaxIterations = 1000000; // 안전장치: 무한루프 방지
			for (size_t i = 0; i < kMaxIterations; ++i)
			{
				if (cursor + 8 > (int64_t)map_data.size())
				{
					break;
				}

				// type_name == 4바이트
				std::string_view type_name(reinterpret_cast<const char*>(&map_data[cursor]), ChunkIDByteStride);

				// data length는 음수가 될 수 있다. (커서를 앞으로 보냄)
				int32_t data_length;
				memcpy(&data_length, &map_data[cursor + ChunkIDByteStride], sizeof(data_length));
				if (data_length < 0)
				{
					ERR_MSG("Invalid data length. Map may be protected. Please check.");
					break;
				}

				const int64_t next_cursor = cursor + (int64_t)ChunkIDByteStride * 2ll + (int64_t)data_length;

				if (0 <= next_cursor && next_cursor <= (int64_t)map_data.size())
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
						auto start = map_data.begin() + cursor + (int64_t)ChunkIDByteStride * 2;
						auto end = start + data_length;
						map_data_table[type_name] = std::vector<uint8_t>(start, end);
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
		std::array<uint32_t, 2> GetMapSizeMegatile(const ChunkTable& map_data_table)
		{
			auto it = map_data_table.find(SCChunkTypeName[(uint32_t)SCChunkType::kMapSize]);
			if (it == map_data_table.end())
			{
				ERR_MSG("DIM chunk not found in map data.");
				return {};
			}

			const std::vector<uint8_t>& dim_data = it->second;
			if (dim_data.size() < 4)
			{
				ERR_MSG("DIM chunk data must exceed 4 bytes.");
				return {};
			}

			uint16_t width = 0;
			uint16_t height = 0;

			memcpy(&width, dim_data.data(), sizeof(uint16_t));
			memcpy(&height, dim_data.data() + sizeof(uint16_t), sizeof(uint16_t));

			if (!(1 <= width && width <= 256 && 1 <= height && height <= 256))
			{
				ERR_MSG("Invalid map size. Width and height must be between 1 and 256.");
				return {};
			}

			return { (uint32_t)width, (uint32_t)height };
		}

		// ERA. 실패 시 TileSetType::kCount
		TileSetType GetTerrainType(const ChunkTable& map_data_table)
		{
			static_assert((int32_t)TileSetType::kCount == 8, "TileSetType::kCount must be 8.");

			auto it = map_data_table.find(SCChunkTypeName[(uint32_t)SCChunkType::kTerrain]);
			if (it == map_data_table.end())
			{
				ERR_MSG("ERA chunk not found in map data.");
				return TileSetType::kCount;
			}

			const std::vector<uint8_t>& era_data = it->second;
			if (era_data.size() < 2)
			{
				ERR_MSG("ERA chunk data must at least 2 bytes.");
				return TileSetType::kCount;
			}

			// 유효 비트는 하위 3비트 (0~7)
			constexpr uint8_t kTerrainMask = 0b00000111;
			return static_cast<TileSetType>(era_data[0] & kTerrainMask);
		}

		// UNIT. 청크가 없으면 빈 vector (맵에 배치 유닛이 없어도 정상)
		std::vector<UnitData> GetUnitData(const ChunkTable& map_data_table)
		{
			auto it = map_data_table.find(SCChunkTypeName[(uint32_t)SCChunkType::kUnitPlacement]);
			if (it == map_data_table.end())
			{
				ERR_MSG("UNIT chunk not found in map data.");
				return {};
			}

			const std::vector<uint8_t>& unit_data_chunk = it->second;

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
		std::vector<MTXM> GetMTXM(const ChunkTable& map_data_table, uint32_t map_width_megatile, uint32_t map_height_megatile)
		{
			auto it = map_data_table.find(SCChunkTypeName[(uint32_t)SCChunkType::kTileMapAtlas]);
			if (it == map_data_table.end())
			{
				ERR_MSG("MTXM chunk not found in map data.");
				return {};
			}

			const std::vector<uint8_t>& mtxm_data = it->second;

			std::vector<MTXM> mtxms((size_t)map_width_megatile * (size_t)map_height_megatile);

			const size_t copy_size = (std::min)(mtxm_data.size(), mtxms.size() * sizeof(MTXM));
			memcpy(mtxms.data(), mtxm_data.data(), copy_size);

			return mtxms;
		}


#pragma region CREEP
		// OpenBW ui.h load_image_data의 v
		constexpr uint8_t ComputeCreepEdgeShape(uint8_t neighbor_mask)
		{
			uint32_t v = 0;
			const uint32_t mask = static_cast<uint32_t>(neighbor_mask);
			if (mask & 2)					v |= 0x10;
			if (mask & 8)					v |= 0x24;
			if (mask & 0x10)				v |= 0x09;
			if (mask & 0x40)				v |= 0x02;
			if ((mask & 0xc0) == 0xc0)		v |= 0x01;
			if ((mask & 0x60) == 0x60)		v |= 0x04;
			if ((mask & 3) == 3)			v |= 0x20;
			if ((mask & 6) == 6)			v |= 0x08;
			if ((v & 0x21) == 0x21)			v |= 0x40;
			if ((v & 0x0c) == 0x0c)			v |= 0x40;
			return static_cast<uint8_t>(v);
		}

		// 이웃 마스크 개수: 8방향 비트 → 256가지
		constexpr size_t kCreepEdgeMaskCount = 256;

		// 모양 코드 개수: ComputeCreepEdgeShape가 OR하는 값이 모두 0x7F 이내 → 0~127
		constexpr size_t kCreepEdgeShapeCount = 128;

		// OpenBW ui.h load_image_data의 creep_edge_frame_index
		// 이웃 마스크(8비트) → 테두리 프레임 번호(0 = 테두리 없음, 1~kCreepEdgeFrameCount = GRP 프레임 번호 + 1)
		//
		// 프레임 번호 = 모양 코드의 순위
		//   256개 마스크에서 실제로 등장하는 모양 코드만 오름차순으로 0, 1, 2, ...를 매긴 값
		//   (테두리 GRP의 프레임이 모양 코드 오름차순으로 저장되어 있다는 전제)
		//   모양 코드 범위가 작아(0~127) 정렬 없이 '등장 여부 표 + 오름차순 순회'로 순위를 구함
		constexpr std::array<uint8_t, kCreepEdgeMaskCount> BuildCreepEdgeFrameIndexTable()
		{
			// 1) 각 모양 코드가 어떤 마스크에서든 한 번이라도 나오는지 기록
			std::array<bool, kCreepEdgeShapeCount> seen = {};
			for (uint32_t mask = 0; mask < kCreepEdgeMaskCount; ++mask)
			{
				seen[ComputeCreepEdgeShape(static_cast<uint8_t>(mask))] = true;
			}

			// 2) 등장한 모양 코드에만 오름차순으로 순위 부여 (오름차순 순회가 곧 정렬)
			//    모양 코드 0(이웃에 Creep 없음)은 항상 등장하므로 순위 0 = 테두리 없음
			std::array<uint8_t, kCreepEdgeShapeCount> rank = {};
			uint8_t next_rank = 0;
			for (uint32_t shape = 0; shape < kCreepEdgeShapeCount; ++shape)
			{
				if (seen[shape])
				{
					rank[shape] = next_rank;
					++next_rank;
				}
			}

			// 3) 마스크 → 모양 코드 → 순위
			std::array<uint8_t, kCreepEdgeMaskCount> table = {};
			for (uint32_t mask = 0; mask < kCreepEdgeMaskCount; ++mask)
			{
				table[mask] = rank[ComputeCreepEdgeShape(static_cast<uint8_t>(mask))];
			}
			return table;
		}

		constexpr std::array<uint8_t, kCreepEdgeMaskCount> kCreepEdgeFrameIndexTable = BuildCreepEdgeFrameIndexTable();

		// 검증: 최대 프레임 번호 = 테두리 GRP 프레임 수
		constexpr uint8_t MaxCreepEdgeFrameIndex()
		{
			uint8_t max_value = 0;
			for (const uint8_t value : kCreepEdgeFrameIndexTable)
			{
				if (value > max_value)
				{
					max_value = value;
				}
			}
			return max_value;
		}
		static_assert(MaxCreepEdgeFrameIndex() == kCreepEdgeFrameCount, "테두리 프레임 번호 범위가 GRP 프레임 수와 다름");

		// 검증: 이웃에 Creep이 없으면 테두리 없음
		static_assert(kCreepEdgeFrameIndexTable[0x00] == 0);

		// 검증: 단일 방향 → 테두리 GRP 프레임 대조 (번호 - 1 = GRP 프레임, 괄호는 로그상 프레임 영역)
		static_assert(kCreepEdgeFrameIndexTable[0x40] == 1);	// 위     → 프레임 0  (0,0)  32x12
		static_assert(kCreepEdgeFrameIndexTable[0x10] == 5);	// 왼쪽   → 프레임 4  (0,0)  12x32
		static_assert(kCreepEdgeFrameIndexTable[0x02] == 7);	// 아래   → 프레임 6  (0,22) 32x10
		static_assert(kCreepEdgeFrameIndexTable[0x08] == 16);	// 오른쪽 → 프레임 15 (20,0) 12x32
		static_assert(kCreepEdgeFrameIndexTable[0xFF] == 37);	// 8방향  → 프레임 36 (0,0)  32x32

		// HLSL에서 리터럴로 만든 테이블(kCreepEdgeFrameIndex)이 OpenBW 규칙으로 만든 표와 같은지 검증
		constexpr bool IsCreepEdgeFrameIndexLiteralValid()
		{
			for (size_t mask = 0; mask < kCreepEdgeMaskCount; ++mask)
			{
				if (kCreepEdgeFrameIndex[mask] != kCreepEdgeFrameIndexTable[mask])
				{
					return false;
				}
			}
			return true;
		}
		static_assert(std::size(kCreepEdgeFrameIndex) == kCreepEdgeMaskCount);
		static_assert(IsCreepEdgeFrameIndexLiteralValid(), "kCreepEdgeFrameIndex 리터럴이 OpenBW 규칙과 다름");

		// OpenBW ui.h init()의 creep_random_tile_indices와 같은 분포
		//   4%: 변형 타일 kCreepTileBaseCount ~ kCreepTileCount - 1 (6~12)
		//   96%: 기본 타일 0 ~ kCreepTileBaseCount - 1 (0~5)
		// 화면용 값이라 시뮬레이션과 무관 → 시드는 시간 기반(OpenBW와 동일하게 실행마다 무늬가 달라짐)
		std::shared_ptr<engine::Texture2D> CreateCreepRandomMegatileTexture(uint32_t map_width_megatile, uint32_t map_height_megatile)
		{
			using namespace engine;

			static_assert(kCreepTileCount <= kCV5MegatilesPerGroup);
			static_assert(kCreepTileCount <= std::numeric_limits<uint8_t>::max());

			const uint64_t seed_source = static_cast<uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count());
			std::mt19937 rng(static_cast<std::mt19937::result_type>(seed_source));

			std::uniform_int_distribution<uint32_t> percent(0, 99);
			std::uniform_int_distribution<uint32_t> base(0, kCreepTileBaseCount - 1);
			std::uniform_int_distribution<uint32_t> variant(0, kCreepTileVariantCount - 1);

			constexpr uint32_t kVariantPercent = 4;

			std::vector<uint8_t> megatiles(static_cast<size_t>(map_width_megatile) * static_cast<size_t>(map_height_megatile));
			for (uint8_t& megatile : megatiles)
			{
				const uint32_t index = (percent(rng) < kVariantPercent)
					? kCreepTileBaseCount + variant(rng)
					: base(rng);
				megatile = static_cast<uint8_t>(index);	// < kCreepTileCount (static_assert)
			}

			D3D11_TEXTURE2D_DESC desc = {};
			desc.Width = map_width_megatile;
			desc.Height = map_height_megatile;
			desc.MipLevels = 1;
			desc.ArraySize = 1;
			desc.Format = DXGI_FORMAT_R8_UINT;
			desc.SampleDesc.Count = 1;
			desc.Usage = D3D11_USAGE_IMMUTABLE;
			desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

			D3D11_SUBRESOURCE_DATA initial_data = {};
			initial_data.pSysMem = megatiles.data();
			initial_data.SysMemPitch = map_width_megatile;	// R8: 1바이트 × width

			std::shared_ptr<Texture2D> texture = EntityManager::CreateEntity<Texture2D>();
			if (!texture->CreateTexture2D(&desc, &initial_data))
			{
				return nullptr;
			}
			return texture;
		}

#pragma endregion CREEP


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

	std::unique_ptr<SCMap> SCMapLoader::LoadSCMap(const std::filesystem::path& map_path)
	{
		std::optional<SCMapLoadingData> map_data = LoadMapLoadingData(map_path);
		if (false == map_data.has_value())
		{
			return nullptr;
		}

		std::unique_ptr<SCMap> result_map = std::make_unique<SCMap>();

		if (FillCommonMapFields(*map_data, *result_map) == false)
		{
			return nullptr;
		}

		return result_map;
	}

	std::shared_ptr<engine::Texture2D> SCMapLoader::LoadMapTextureGPU(const std::filesystem::path& map_path)
	{
		ASSERT(sc_map_baker_ != nullptr);

		std::optional<SCMapLoadingData> map_data = LoadMapLoadingData(map_path);
		if (false == map_data.has_value())
		{
			return nullptr;
		}

		std::shared_ptr<engine::Texture2D> texture = sc_map_baker_->BakeMapTexture(*map_data);
		if (texture == nullptr)
		{
			return nullptr;
		}
		return texture;
	}

	std::shared_ptr<engine::Texture2D> SCMapLoader::LoadMapTextureCPU(const std::filesystem::path& map_path)
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
		const size_t map_width = (size_t)map_data.map_width_megatile * (size_t)32;
		const size_t map_height = (size_t)map_data.map_height_megatile * (size_t)32;
		std::vector<RGBA> map_pixels(map_width * map_height);

		// 채워넣기
		for (uint32_t mega_y = 0; mega_y < map_data.map_height_megatile; ++mega_y)
		{
			for (uint32_t mega_x = 0; mega_x < map_data.map_width_megatile; ++mega_x)
			{
				// 1. MTXM에서 타일 정보 가져오기
				const uint16_t v = map_data.mtxm[mega_y * map_data.map_width_megatile + mega_x];

				// CV5 인덱스
				const uint32_t group = MTXMGroup(v);

				// CV5 내 타일 인덱스 (0 ~ 15)
				const uint32_t cv5_sub_idx = MTXMTile(v);

				// 널 타일 → 검정
				if (group >= tileset.cv5.size()) { continue; }

				// CV5[group]의 CV5_megatiles[cv5_sub_idx]
				const uint16_t mega = tileset.cv5[group].megatiles[cv5_sub_idx];
				if (mega >= tileset.vx4.size()) { continue; }

				// 메가타일 안의 미니타일 4*4 참조
				for (uint32_t mini = 0; mini < 16; ++mini)
				{
					// VX4에서 VR4 참조 및 반전 여부 확인
					const uint16_t ref = tileset.vx4[mega].minitiles[mini];
					const uint16_t vr4_id = VX4Ref(ref);
					const bool   flipped = VX4Flipped(ref);
					if (vr4_id >= tileset.vr4.size()) { continue; }

					const uint32_t mini_x = (mini % 4) * 8;
					const uint32_t mini_y = (mini / 4) * 8;

					for (uint32_t py = 0; py < 8; ++py)
					{
						for (uint32_t px = 0; px < 8; ++px)
						{
							// VR4에서 WPE 색인 가져오기
							const uint8_t idx = tileset.vr4[vr4_id].pixels[py][flipped ? 7 - px : px];

							// 최종 픽셀 좌표 계산
							const uint32_t x = mega_x * 32 + mini_x + px;
							const uint32_t y = mega_y * 32 + mini_y + py;

							const auto& color = tileset.wpe[idx];
							RGBA& pixel = map_pixels[y * map_width + x];

							pixel.r = color.r;
							pixel.g = color.g;
							pixel.b = color.b;
							pixel.a = 255; // WPE의 a값은 미사용
						}
					}
				}
			}
		}

		// Texture 생성
		constexpr DXGI_FORMAT kTextureFormat = DXGI_FORMAT_R8G8B8A8_UNORM;	// WPE 색인 → RGBA 변환은 쉐이더에서 처리
		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width = (UINT)map_width;
		desc.Height = (UINT)map_height;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = kTextureFormat;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_IMMUTABLE;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

		D3D11_SUBRESOURCE_DATA init_data = {};
		init_data.pSysMem = map_pixels.data();
		init_data.SysMemPitch = (uint32_t)(GetDXGIFormatByteStride(kTextureFormat) * (uint32_t)map_width);      // 바이트 단위
		
		std::shared_ptr<Texture2D> texture = EntityManager::CreateEntity<Texture2D>();
		const bool result = texture->CreateTexture2D(&desc, &init_data);
		if (result == false)
		{
			return nullptr;
		}

		return texture;
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
				!read_into(each_tileset_path, ".VF4", tileset.vf4) ||
				!read_into(each_tileset_path, ".GRP", tileset.creep_grp_bytes)
				)
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

		auto ClearTilesets = [&]()
			{
				for (TileSetDataGPU& gpu : tileset_gpu_buffers_)
				{
					gpu = TileSetDataGPU();
				}
			};

		std::vector<uint16_t> megatile_data;
		bool result = false;

		for (size_t tileset_i = 0; tileset_i < tileset_gpu_buffers_.size(); ++tileset_i)
		{
			TileSetData& tileset = tilesets_[tileset_i];
			tileset_gpu_buffers_[tileset_i] = TileSetDataGPU();
			TileSetDataGPU& tileset_gpu = tileset_gpu_buffers_[tileset_i];

#pragma region CV5 - Megatile
			// CV5 구조체의 CV5_megatiles 배열 크기와 오프셋을 컴파일 타임에 검증
			static_assert(CV5{}.megatiles.size() == kCV5MegatilesPerGroup);
			static_assert(offsetof(CV5, megatiles) == 20);
			if (tileset.cv5.empty())
			{
				ERR_MSG("CV5 data is empty for current tileset index");
				ClearTilesets();
				return false;
			}
			megatile_data.resize(tileset.cv5.size() * kCV5MegatilesPerGroup);

			// CV5에서 megatile index만 추출하여 LUT 생성
			for (size_t j = 0; j < tileset.cv5.size(); ++j)
			{
				std::ranges::copy(tileset.cv5[j].megatiles, megatile_data.begin() + j * kCV5MegatilesPerGroup);
			}

			tileset_gpu.CV5_megatiles = EntityManager::CreateEntity<TypedBuffer>();
			std::span<const uint16_t> megatile_span = megatile_data;
			result = tileset_gpu.CV5_megatiles->CreateImmutableBuffer(DXGI_FORMAT_R16_UINT, megatile_span);
			if (!result) { ClearTilesets(); return false; }

			// 크립용 CV5 데이터가 있는지 확인
			if (tileset.cv5.size() <= kCreepCV5Group)
			{
				ERR_MSG("CV5 group count is less than required for creep. ({} < {})", tileset.cv5.size(), kCreepCV5Group + 1);
				ClearTilesets();
				return false;
			}
#pragma endregion CV5 - Megatile

#pragma region VX4 - Minitile
			static_assert(sizeof(VX4) == sizeof(uint16_t) * minitiles_per_VX4);
			if (tileset.vx4.empty())
			{
				ERR_MSG("VX4 data is empty for tileset index");
				ClearTilesets();
				return false;
			}
			tileset_gpu.VX4_minitiles = EntityManager::CreateEntity<TypedBuffer>();
			result = tileset_gpu.VX4_minitiles->CreateImmutableBuffer(DXGI_FORMAT_R16_UINT, sizeof(VX4::Elem), tileset.vx4.data(), (uint32_t)(tileset.vx4.size() * minitiles_per_VX4));
			if (!result) { ClearTilesets(); return false; }
#pragma endregion VX4 - Minitile

#pragma region VR4 - Minitile WPE index
			if (tileset.vr4.empty())
			{
				ERR_MSG("VR4 data is empty for current tileset index");
				ClearTilesets();
				return false;
			}
			tileset_gpu.VR4_minitile_wpe_indices = EntityManager::CreateEntity<TypedBuffer>();
			result = tileset_gpu.VR4_minitile_wpe_indices->CreateImmutableBuffer(DXGI_FORMAT_R8_UINT, sizeof(VR4::Elem), tileset.vr4.data(), (uint32_t)(tileset.vr4.size() * minile_pixel_count));
			if (!result) { ClearTilesets(); return false; }
#pragma endregion VR4 - Minitile WPE index

#pragma region WPE - Color Palette
			if (tileset.wpe.size() != 256)
			{
				ERR_MSG("WPE data is empty or invalid for current tileset index");
				ClearTilesets();
				return false;
			}
			tileset_gpu.WPE_color_palettes = EntityManager::CreateEntity<TypedBuffer>();
			std::span<const WPE> wpe_span = tileset.wpe;
			result = tileset_gpu.WPE_color_palettes->CreateImmutableBuffer(DXGI_FORMAT_R8G8B8A8_UNORM, wpe_span);
			if (!result) { ClearTilesets(); return false; }
#pragma endregion WPE - Color Palette

#pragma region		Cloak fade selector
			// OpenBW ui.h cloak_fade_selector: WPE 색별 밝기 등급(0~8)
			std::vector<uint8_t> cloak_fade_selector;
			cloak_fade_selector.resize(wpe_span.size());
			for (size_t i = 0; i < wpe_span.size(); ++i)
			{
				const WPE& wpe = wpe_span[i];
				cloak_fade_selector[i] = static_cast<uint8_t>(
					(static_cast<uint32_t>(wpe.r) * kCloakFadeSelectorWeightR + 
						static_cast<uint32_t>(wpe.g) * kCloakFadeSelectorWeightG + 
						static_cast<uint32_t>(wpe.b) * kCloakFadeSelectorWeightB + kCloakFadeSelectorRound) / kCloakFadeSelectorDivisor);
			}

			tileset_gpu.cloak_fade_selector = EntityManager::CreateEntity<TypedBuffer>();
			result = tileset_gpu.cloak_fade_selector->CreateImmutableBuffer(DXGI_FORMAT_R8_UINT, cloak_fade_selector);
			if (!result) { ClearTilesets(); return false; }
#pragma endregion	Cloak fade selector

#pragma region PCX - Tileset LUT
			{
				ASSERT(tileset.pcx_lut);
				std::shared_ptr<TypedBuffer>& pcx_lut_gpu = tileset_gpu.PCX_tileset_LUT;
				pcx_lut_gpu = EntityManager::CreateEntity<TypedBuffer>();

				std::span<const SCTileSetPcxLUT::ElemType> pcx_lut_span = tileset.pcx_lut->data();
				result = pcx_lut_gpu->CreateImmutableBuffer(DXGI_FORMAT_R8_UINT, pcx_lut_span);
				if (!result) { ClearTilesets(); return false; }

				
				std::shared_ptr<TypedBuffer>& pcx_lut_offset_gpu = tileset_gpu.PCX_tileset_LUT_offsets;
				pcx_lut_offset_gpu = EntityManager::CreateEntity<TypedBuffer>();

				std::span<const SCTileSetPcxLUT::TableRange> pcx_lut_offset_span = tileset.pcx_lut->ranges();

				result = pcx_lut_offset_gpu->CreateImmutableBuffer(DXGI_FORMAT_R32G32_UINT, pcx_lut_offset_span);
				if (!result) { ClearTilesets(); return false; }
			}
#pragma endregion PCX - Tileset LUT

#pragma region		GRP - Creep
			{
				ASSERT(tileset.creep_grp_bytes.empty() == false);
				std::vector<uint8_t> pixels;
				BakedGrp grp = SCImageLibrary::BakeGrp(std::move(tileset.creep_grp_bytes), pixels);
				
				if (grp.texture == nullptr)
				{
					ERR_MSG("Failed to bake Creep GRP for tileset index {}", tileset_i);
					ClearTilesets();
					return false;
				}
				
				CHECK_F(grp.texture->GetFrameCount() == kCreepEdgeFrameCount, "Creep GRP frame count must be 37");
				CHECK_F(grp.texture->GetWidth() == kCreepEdgeCanvasSize && grp.texture->GetHeight() == kCreepEdgeCanvasSize, 
					"Creep GRP canvas size must be 32x32");

				tileset_gpu.creep_edge = std::move(grp.texture);
			}
#pragma endregion	GRP - Creep
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
		const std::array<uint32_t, 2> map_size_megatile = GetMapSizeMegatile(map_data.map_data_table);
		if (map_size_megatile[0] == 0 || map_size_megatile[1] == 0)
		{
			return std::nullopt;
		}
		map_data.map_width_megatile = map_size_megatile[0];
		map_data.map_height_megatile = map_size_megatile[1];

		// 지형 타입
		const TileSetType terrain_type = GetTerrainType(map_data.map_data_table);
		if (terrain_type == TileSetType::kCount)
		{
			return std::nullopt;
		}
		map_data.terrain_type = terrain_type;

		// MTXM 데이터
		map_data.mtxm = GetMTXM(map_data.map_data_table, map_data.map_width_megatile, map_data.map_height_megatile);
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

	bool SCMapLoader::FillCommonMapFields(SCMapLoadingData& map_data, SCMap& out_map) const
	{
		ASSERT(map_data.terrain_type < TileSetType::kCount);

		out_map.creep_random_megatiles = CreateCreepRandomMegatileTexture(map_data.map_width_megatile, map_data.map_height_megatile);

		if (out_map.creep_random_megatiles == nullptr)
		{
			ERR_MSG("Failed to create Creep random megatile texture.");
			out_map = SCMap{};
			return false;
		}

		if(map_data.mtxm.size() != map_data.map_width_megatile * map_data.map_height_megatile)
		{
			ERR_MSG("MTXM size does not match megatile dimensions.");
			out_map = SCMap{};
			return false;
		}
		out_map.mtxm = engine::EntityManager::CreateEntity<engine::TypedBuffer>();
		bool result = out_map.mtxm->CreateImmutableBuffer(DXGI_FORMAT_R16_UINT, map_data.mtxm);
		if (!result)
		{
			ERR_MSG("Failed to create MTXM buffer.");
			out_map = SCMap{};
			return false;
		}

		const TileSetDataGPU& tileset_gpu = tileset_gpu_buffers_[(size_t)map_data.terrain_type];
		out_map.map_width_megatile = map_data.map_width_megatile;
		out_map.map_height_megatile = map_data.map_height_megatile;
		out_map.terrain_type = map_data.terrain_type;
		out_map.unit_data = std::move(map_data.unit_data);
		out_map.wpe_color_palettes = tileset_gpu.WPE_color_palettes;
		out_map.pcx_tileset_LUT = tileset_gpu.PCX_tileset_LUT;
		out_map.pcx_tileset_LUT_offsets = tileset_gpu.PCX_tileset_LUT_offsets;
		out_map.cloak_fade_selector = tileset_gpu.cloak_fade_selector;
		out_map.creep_edge = tileset_gpu.creep_edge;
		out_map.cv5_megatiles = tileset_gpu.CV5_megatiles;
		out_map.vx4_minitiles = tileset_gpu.VX4_minitiles;
		out_map.vr4_minitile_wpe_indices = tileset_gpu.VR4_minitile_wpe_indices;

		return true;
	}
}