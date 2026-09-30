#pragma once
#include <Engine/Resource/GPU/Shader/ComputePass.h>

#include <Content/SCData/SCMapCommon.h>

struct ID3D11DeviceContext;

namespace engine
{
	class Texture2D;
	class ConstantBuffer;
	class TypedBuffer;
}

namespace sc
{	
	struct TileSetGPU
	{
		std::unique_ptr<engine::TypedBuffer> CV5_megatiles = {};	// from CV5
		std::unique_ptr<engine::TypedBuffer> VX4_minitiles = {};	// from VX4
		std::unique_ptr<engine::TypedBuffer> VR4_minitile_wpe_indices = {}; // from VR4
		std::shared_ptr<engine::TypedBuffer> WPE_color_palettes = {}; // from WPE, std::shared_ptr - 렌더링할 때 필요하므로 공유 자원이여야 함
		std::shared_ptr<engine::TypedBuffer> PCX_tileset_LUT = {}; // from SCTileSetPcxLUT, std::shared_ptr - 렌더링할 때 필요하므로 공유 자원이여야 함
		std::shared_ptr<engine::ConstantBuffer> PCX_tileset_LUT_offsets = {}; // from SCTileSetPcxLUT, std::shared_ptr - 렌더링할 때 필요하므로 공유 자원이여야 함
	};

    class SCMapBakeComputePass :
        public engine::ComputePass
    {
		ENTITY_INFO(SCMapBakeComputePass, engine::ComputePass)
    public:
        SCMapBakeComputePass();
		virtual ~SCMapBakeComputePass() override;
		
		virtual std::shared_ptr<engine::ComputeShader> LoadComputeShader() override;
		bool CreateTileSetGPUData(const std::array<TileSet, (size_t)TileSetType::kEND>& tileset_data);

		std::shared_ptr<engine::Texture2D> BakeMapTexture(const SCMapLoadingData& map_data);

		std::shared_ptr<engine::TypedBuffer> GetWPEColorPaletteBuffer(TileSetType terrain_type) const {
			if ((size_t)terrain_type >= tileset_gpu_buffers_.size()) { return nullptr; }
			return tileset_gpu_buffers_[(size_t)terrain_type].WPE_color_palettes;
		}

		std::shared_ptr<engine::TypedBuffer> GetPCXTilesetLUTBuffer(TileSetType terrain_type) const {
			if ((size_t)terrain_type >= tileset_gpu_buffers_.size()) { return nullptr; }
			return tileset_gpu_buffers_[(size_t)terrain_type].PCX_tileset_LUT;
		}

	protected:
		virtual void BindResources(ID3D11DeviceContext* context) override;
		virtual void UnbindResources(ID3D11DeviceContext* context) override;
		virtual std::array<UINT, 3> GetThreadCount() const override;

	private:
		std::unique_ptr<engine::ConstantBuffer> map_info_buffer_ = {};
		std::array<TileSetGPU, (size_t)TileSetType::kEND> tileset_gpu_buffers_ = {};
		
		std::array<UINT, 3> thread_count_ = { 0, 0, 0 };
    };
}


