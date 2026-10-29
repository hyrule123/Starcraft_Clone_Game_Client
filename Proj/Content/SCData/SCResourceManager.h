#pragma once
#include <Engine/Core/CoreMinimal.h>

#include <Engine/Core/Singleton.h>

#include <Content/SCData/Image/SCSpriteCommon.h>
#include <Content/SCData/Format/MPQ/MPQFileSystem.h>
#include <Content/SCData/Image/SCImageLibrary.h>
#include <Content/SCData/Map/SCMapLoader.h>

#include <array>

namespace engine
{
	class Material;
	class Mesh;
	class DepthStencilState;
	class TypedBuffer;
	class InputLayout;
	class RasterizerState;
}

namespace sc
{
	class SCResourceManager
	{
		DECLARE_SINGLETON(SCResourceManager)
	public:
		void Init();
		bool HasInitialized() const { return has_initialized_; }

		const std::filesystem::path& GetMPQDir() const { return mpq_dir_; }

		SCMapLoader& GetMapLoader() { return sc_map_loader_; }

		const sc::SCImageLibrary& GetSCImageLibrary() const { return sc_image_library_; }

		std::shared_ptr<engine::Material> GetImageMaterial(int32_t image_type, SCImageVariant variant);
		std::shared_ptr<engine::Mesh> GetImageMesh() const { return image_mesh_; }

		std::shared_ptr<engine::Mesh> GetMapMesh() const { return map_mesh_; }
		std::shared_ptr<engine::Material> GetNewMapMaterial() const;

		// 체력 바 공용 Material (텍스처 없음 → 모든 바가 공유, 인스턴싱으로 한 번에 그림)
		std::shared_ptr<engine::Material> GetHpBarMaterial() const { return hp_bar_material_; }

	private:
		void CreateCommonPaletteBuffer(const MPQFileSystem& mpq_fs);
		void CreateDepthStencilStates();
		void CreateOriginalMaterials();
		void CreateHpBarMaterial();
		void CreateMesh();
		void CreateMapMaterial();

	private:
		// 리소스가 저장되어 있는 MPQ 모음
		MPQFileSystem mpq_file_system_ = {};

		SCMapLoader sc_map_loader_ = {};

		// GRP 이미지 라이브러리
		SCImageLibrary sc_image_library_ = {};

		// GRP 별 Material, modifier에 따라 다른 Material을 사용해야 함
		std::vector<std::array<std::shared_ptr<engine::Material>, (size_t)SCImageVariant::kCount>> materials_ = {};
		std::array<std::shared_ptr<engine::Material>, (size_t)SCImageVariant::kCount> orig_materials_ = {};
		std::shared_ptr<engine::Mesh> image_mesh_ = {};

		std::shared_ptr<engine::Mesh> map_mesh_ = {};
		std::shared_ptr<engine::Material> map_material_ = {};

		// 체력 바
		std::shared_ptr<engine::Material> hp_bar_material_ = {};

		// 이미지 파이프라인 공용 상태 (체력 바 파이프라인도 재사용 — 같은 ImageVSInput)
		std::shared_ptr<engine::InputLayout> image_input_layout_ = {};
		std::shared_ptr<engine::RasterizerState> image_rss_ = {};



		// GPU
		std::shared_ptr<engine::DepthStencilState> depth_key_write_dss_ = {};
		std::shared_ptr<engine::DepthStencilState> depth_key_test_dss_ = {};



		// Common
		std::shared_ptr<engine::TypedBuffer> common_palettes_buffer_ = {};

		// etc
		std::filesystem::path mpq_dir_ = {};
		bool has_initialized_ = false;
	};
}


