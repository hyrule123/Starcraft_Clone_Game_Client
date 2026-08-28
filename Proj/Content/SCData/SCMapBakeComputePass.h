#pragma once
#include <Engine/Resource/ComputePass.h>

#include <Content/SCData/SCMapCommon.h>

struct ID3D11DeviceContext;
namespace engine
{
	class Texture2D;
	class ConstantBuffer;
	class TypedBuffer;

	class TypedBuffer;
	struct TileSetGPUData
	{
		u_ptr<TypedBuffer> CV5_megatiles = {};	// from CV5
		u_ptr<TypedBuffer> VX4_minitiles = {};	// from VX4
		u_ptr<TypedBuffer> VR4_minitile_wpe_indices = {}; // from VR4
		u_ptr<TypedBuffer> WPE_color_palettes = {}; // from WPE
	};

    class SCMapBakeComputePass :
        public ComputePass
    {
		ENTITY_INFO(SCMapBakeComputePass, ComputePass)
    public:
        SCMapBakeComputePass();
		virtual ~SCMapBakeComputePass() override;
		
		virtual s_ptr<ComputeShader> LoadComputeShader() override;
		bool CreateTileSetGPUData(const std::vector<TileSet>& tileset_data);

		s_ptr<Texture2D> BakeMapTexture(const MapInfo& map_info);

	protected:
		virtual void BindResources(ID3D11DeviceContext* context) override;
		virtual void UnbindResources(ID3D11DeviceContext* context) override;
		virtual std::array<UINT, 3> GetThreadCount() const override;

	private:
		u_ptr<ConstantBuffer> map_info_buffer_ = {};
		std::vector<TileSetGPUData> tileset_gpu_data_ = {};
		
		MapInfo map_info_ = {};
		std::array<UINT, 3> thread_count_ = { 0, 0, 0 };

		s_ptr<Texture2D> map_texture_ = {};
    };
}


