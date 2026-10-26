#include "Content/pch.h"
#include "SCMapBakeComputePass.h"

#include <Engine/Core/Debug.h>
#include <Engine/Core/DX11.h>

#include <Engine/Resource/GPU/Shader/ComputeShader.h>
#include <Engine/Resource/GPU/Buffer/Texture2D.h>
#include <Engine/Resource/GPU/Buffer/ConstantBuffer.h>
#include <Engine/Resource/GPU/Buffer/TypedBuffer.h>

#include <Engine/Manager/ResourceManager.h>
#include <Engine/Manager/GraphicsDevice.h>

#include <Content/HLSL/SCMap/SCMapBaker.hlsli>

#include <Content/SCData/SCTileSetPcxLUT.h>

#include <span>

namespace sc
{
	SCMapBakeComputePass::SCMapBakeComputePass()
		: Super(SCMapBakeComputePass::kClassConcreteName)
	{}
	SCMapBakeComputePass::~SCMapBakeComputePass()
	{}
	std::shared_ptr<engine::ComputeShader> SCMapBakeComputePass::LoadComputeShader()
	{
		std::shared_ptr<engine::ComputeShader> cs = engine::ResourceManager::GetInst().LoadFromFile<engine::ComputeShader>("Shader/SCMapBaker_CS.cso");
		ASSERT(cs != nullptr);
		return cs;
	}

	std::shared_ptr<engine::Texture2D> SCMapBakeComputePass::BakeMapTexture(const SCMapLoadingData& map_data)
	{
		using namespace engine;

		if (map_data.map_width_megatile == 0 || map_data.map_height_megatile == 0)
		{
			ERR_MSG("Invalid map size. Width and height must be greater than 0.");
			return nullptr;
		}
		if (map_data.mtxm.size() < (map_data.map_width_megatile * map_data.map_height_megatile))
		{
			ERR_MSG("MTXM data is invalid.");
			return nullptr;
		}

		CHECK_F(map_data.tileset_gpu != nullptr, "TileSet GPU data is empty.");
		const auto& tileset_gpu = *(map_data.tileset_gpu);
		if (
			tileset_gpu.CV5_megatiles == nullptr ||
			tileset_gpu.VX4_minitiles == nullptr ||
			tileset_gpu.VR4_minitile_wpe_indices == nullptr)
		{
			ERR_MSG("TileSet GPU data is not created for the specified terrain type.");
			return nullptr;
		}

		bool result = false;

		auto context = GraphicsDevice::GetInst().GetContext();

		MapInfoCB map_info_cb = {};
		map_info_cb.map_size_megatile = { (int32_t)map_data.map_width_megatile, (int32_t)map_data.map_height_megatile };
		
		if (map_info_buffer_ == nullptr)
		{
			map_info_buffer_ = EntityManager::CreateEntity<ConstantBuffer>();
			result = map_info_buffer_->Create<MapInfoCB>();
			if (!result) { return nullptr; }
		}
		map_info_buffer_->Upload(context, map_info_cb);

		// CB bind
		map_info_buffer_->Bind(context, ShaderStage::Compute, REG_B_MAP_INFO);

		std::shared_ptr<Texture2D> map_texture = EntityManager::CreateEntity<Texture2D>();
		D3D11_TEXTURE2D_DESC desc = {};
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		desc.Width = map_data.map_width_megatile * (UINT)kMegatilePixelSize;
		desc.Height = map_data.map_height_megatile * (UINT)kMegatilePixelSize;

		result = map_texture->CreateTexture2D(&desc, nullptr);
		if (!result) { return nullptr; }

		// scx 맵 데이터(mtxm) 생성
		std::unique_ptr<TypedBuffer> mtxm_buffer = EntityManager::CreateEntity<TypedBuffer>();
		std::span<const MTXM> mtxm_span = map_data.mtxm;
		result = mtxm_buffer->CreateImmutableBuffer(DXGI_FORMAT_R16_UINT, mtxm_span);
		if (!result) { return nullptr; }

		// Tileset Bind
		tileset_gpu.CV5_megatiles->BindSRV(context, ShaderStage::Compute, REG_T_TILESET_CV5);
		tileset_gpu.VX4_minitiles->BindSRV(context, ShaderStage::Compute, REG_T_TILESET_VX4);
		tileset_gpu.VR4_minitile_wpe_indices->BindSRV(context, ShaderStage::Compute, REG_T_TILESET_VR4);
		tileset_gpu.WPE_color_palettes->BindSRV(context, ShaderStage::Compute, REG_T_TILESET_WPE);

		// Mtxm Bind
		mtxm_buffer->BindSRV(context, ShaderStage::Compute, REG_T_MAP_MTXM);

		// Texture Bind
		map_texture->BindUAV(context, REG_U_MAP_TEXTURE);

		thread_count_ = { desc.Width, desc.Height, 1 };

		Execute(context);

		map_texture->UnbindUAV(context, REG_U_MAP_TEXTURE);
		
		mtxm_buffer->UnbindSRV(context, ShaderStage::Compute, REG_T_MAP_MTXM);

		tileset_gpu.WPE_color_palettes->UnbindSRV(context, ShaderStage::Compute, REG_T_TILESET_WPE);
		tileset_gpu.VR4_minitile_wpe_indices->UnbindSRV(context, ShaderStage::Compute, REG_T_TILESET_VR4);
		tileset_gpu.VX4_minitiles->UnbindSRV(context, ShaderStage::Compute, REG_T_TILESET_VX4);
		tileset_gpu.CV5_megatiles->UnbindSRV(context, ShaderStage::Compute, REG_T_TILESET_CV5);

		return map_texture;
	}

	void SCMapBakeComputePass::BindResources(ID3D11DeviceContext* context)
	{
		// Bind/Unbind는 BakeMapTexture에서 처리하므로 여기서는 아무것도 하지 않음
	}
	void SCMapBakeComputePass::UnbindResources(ID3D11DeviceContext* context)
	{
		// Bind/Unbind는 BakeMapTexture에서 처리하므로 여기서는 아무것도 하지 않음
	}
	std::array<UINT, 3> SCMapBakeComputePass::GetThreadCount() const
	{
		return thread_count_;
	}
}