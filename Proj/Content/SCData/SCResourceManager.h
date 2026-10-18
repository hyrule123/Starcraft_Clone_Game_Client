#pragma once
#include <Engine/Core/CoreMinimal.h>

#include <Engine/Core/Singleton.h>

#include <Content/SCData/SCSpriteCommon.h>
#include <Content/SCData/MPQ/MPQFileSystem.h>
#include <Content/SCData/SCImageLibrary.h>
#include <Content/SCData/SCMapLoader.h>

#include <array>

namespace engine
{
	class Material;
	class Mesh;
	class DepthStencilState;
	class TypedBuffer;
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

		std::shared_ptr<engine::Material> GetImageMaterial(uint32_t image_type, SCImageMaterialVariant variant);
		std::shared_ptr<engine::Mesh> GetImageMesh() const { return image_mesh_; }

		std::shared_ptr<engine::Mesh> GetMapMesh() const { return map_mesh_; }
		std::shared_ptr<engine::Material> GetNewMapMaterial() const;

	private:
		void CreateTunitBuffer(const MPQFileSystem& mpq_fs);
		void CreateDepthStencilStates();
		void CreateOriginalMaterials();
		void CreateMesh();
		void CreateMapMaterial();

	private:
		// 리소스가 저장되어 있는 MPQ 모음
		MPQFileSystem mpq_file_system_ = {};

		SCMapLoader sc_map_loader_ = {};

		// GRP 이미지 라이브러리
		SCImageLibrary sc_image_library_ = {};

		// GRP 별 Material, modifier에 따라 다른 Material을 사용해야 함
		std::vector<std::array<std::shared_ptr<engine::Material>, (size_t)SCImageMaterialVariant::kCount>> materials_ = {};
		std::array<std::shared_ptr<engine::Material>, (size_t)SCImageMaterialVariant::kCount> orig_materials_ = {};
		std::shared_ptr<engine::Mesh> image_mesh_ = {};

		// GPU
		std::shared_ptr<engine::DepthStencilState> depth_key_write_dss_ = {};
		std::shared_ptr<engine::DepthStencilState> depth_key_test_dss_ = {};

		std::shared_ptr<engine::Mesh> map_mesh_ = {};
		std::shared_ptr<engine::Material> map_material_ = {};

		// Common
		std::shared_ptr<engine::TypedBuffer> tunit_buffer_ = {};

		// etc
		std::filesystem::path mpq_dir_;
		bool has_initialized_ = false;
	};
}


