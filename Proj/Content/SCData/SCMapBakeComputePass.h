#pragma once
#include <Engine/Resource/GPU/Shader/ComputePass.h>

#include <Content/SCData/SCMapCommon.h>

struct ID3D11DeviceContext;
namespace engine
{
	class Texture2D;
	class ConstantBuffer;
	class TypedBuffer;

	class TypedBuffer;
	struct TileSetGPU
	{
		u_ptr<TypedBuffer> CV5_megatiles = {};	// from CV5
		u_ptr<TypedBuffer> VX4_minitiles = {};	// from VX4
		u_ptr<TypedBuffer> VR4_minitile_wpe_indices = {}; // from VR4
		s_ptr<TypedBuffer> WPE_color_palettes = {}; // from WPE, s_ptr - 렌더링할 때 필요하므로 공유 자원이여야 함
	};

    class SCMapBakeComputePass :
        public ComputePass
    {
		ENTITY_INFO(SCMapBakeComputePass, ComputePass)
    public:
        SCMapBakeComputePass();
		virtual ~SCMapBakeComputePass() override;
		
		virtual s_ptr<ComputeShader> LoadComputeShader() override;
		bool CreateTileSetGPUData(const std::array<TileSet, (size_t)TileSetType::kEND>& tileset_data);

		s_ptr<Texture2D> BakeMapTexture(const SCMapLoadingData& map_data);

		s_ptr<TypedBuffer> GetWPEColorPaletteBuffer(TileSetType terrain_type) const {
			if ((size_t)terrain_type >= tileset_gpu_buffers_.size()) { return nullptr; }
			return tileset_gpu_buffers_[(size_t)terrain_type].WPE_color_palettes;
		}

	protected:
		virtual void BindResources(ID3D11DeviceContext* context) override;
		virtual void UnbindResources(ID3D11DeviceContext* context) override;
		virtual std::array<UINT, 3> GetThreadCount() const override;

	private:
		u_ptr<ConstantBuffer> map_info_buffer_ = {};
		std::array<TileSetGPU, (size_t)TileSetType::kEND> tileset_gpu_buffers_ = {};
		
		std::array<UINT, 3> thread_count_ = { 0, 0, 0 };
    };
}


