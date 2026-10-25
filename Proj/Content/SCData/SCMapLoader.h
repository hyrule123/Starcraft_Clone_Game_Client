#pragma once
#include <Engine/Core/CoreMinimal.h>

#include <Content/SCData/SCMapCommon.h>

#include <array>
#include <filesystem>
#include <memory>
#include <optional>

namespace engine
{
	class TypedBuffer;
}

namespace sc
{
	class MPQFileSystem;
	class SCMapBakeComputePass;

	// 맵 파일(.scm/.scx) 로딩 담당.
	// - 앱 수명 동안 8종 타일셋의 CPU/GPU 데이터와 맵 베이커를 보유한다.
	// - SCResourceManager가 값 멤버로 소유하며, Init은 SCResourceManager::Init에서 1회 호출한다.
	class SCMapLoader
	{
	public:
		SCMapLoader();
		~SCMapLoader();

		SCMapLoader(const SCMapLoader&) = delete;
		SCMapLoader& operator=(const SCMapLoader&) = delete;

		// MPQ 파일 시스템은 초기화(타일셋 로드)에만 사용하고 보관하지 않는다.
		void Init(const MPQFileSystem& mpq_file_system);

		// 컴퓨트 셰이더로 맵 인덱스 텍스처를 굽는다. 실패 시 nullptr
		std::unique_ptr<SCMap> LoadMapGPU(const std::filesystem::path& map_path);

		// CPU에서 맵 인덱스 텍스처를 굽는다 (디버그·검증용). 실패 시 nullptr
		std::unique_ptr<SCMap> LoadMapCPU(const std::filesystem::path& map_path);

	private:
		bool LoadTilesetData(const MPQFileSystem& mpq_file_system);
		bool CreateTileSetGPUData();

		// 맵 MPQ에서 scenario.chk를 읽어 파싱. 실패 시 nullopt
		std::optional<SCMapLoadingData> LoadMapLoadingData(const std::filesystem::path& map_path);

		// 로딩 결과 공통 필드(크기, 지형, 유닛, 팔레트/LUT 버퍼)를 SCMap에 채운다
		// 실패 시 false. map_data, out_map은 비워짐.
		[[nodiscard]] bool FillCommonMapFields(SCMapLoadingData& map_data, SCMap& out_map) const;

	private:
		// Index = TileSet Type (Jungle, Desert, Ice, Twilight, etc.)
		std::array<TileSetData, (size_t)TileSetType::kCount> tilesets_ = {};
		std::array<TileSetDataGPU, (size_t)TileSetType::kCount> tileset_gpu_buffers_ = {};

		std::unique_ptr<SCMapBakeComputePass> sc_map_baker_ = {};

		bool has_initialized_ = false;
	};
}