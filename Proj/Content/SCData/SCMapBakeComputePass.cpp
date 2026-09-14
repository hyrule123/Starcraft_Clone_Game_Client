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

#include <Content/HLSL/SCMapTileSet.hlsli>

#include <span>

namespace engine
{
	SCMapBakeComputePass::SCMapBakeComputePass()
		: Super(SCMapBakeComputePass::kClassConcreteName)
	{}
	SCMapBakeComputePass::~SCMapBakeComputePass()
	{}
	s_ptr<ComputeShader> SCMapBakeComputePass::LoadComputeShader()
	{
		s_ptr<ComputeShader> cs = ResourceManager::GetInst().LoadFromFile<ComputeShader>("Shader/SCMapBaker_CS.cso"_hash);
		ASSERT(cs != nullptr);
		return cs;
	}
	bool SCMapBakeComputePass::CreateTileSetGPUData(const std::array<TileSet, (size_t)TileSetType::kEND>& tileset_data)
	{
		auto clear_tilesets = [&]() {
			for (size_t i = 0; i < tileset_gpu_buffers_.size(); ++i)
			{
				tileset_gpu_buffers_[i] = TileSetGPU();
			}
			};

		std::vector<uint16> megatile_data;
		bool result = false;

		//index tileset
		for (size_t tileset_i = 0; tileset_i < tileset_gpu_buffers_.size(); ++tileset_i)
		{
			const TileSet& tileset = tileset_data[tileset_i];
			tileset_gpu_buffers_[tileset_i] = TileSetGPU();
			TileSetGPU& tileset_gpu = tileset_gpu_buffers_[tileset_i];

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
				std::ranges::copy(tileset.cv5[j].megatiles,
					megatile_data.begin() + j * megatiles_per_group);
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
			if (tileset.wpe.empty() || tileset.wpe.size() != 256)
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
		}

		return true;
	}
	s_ptr<Texture2D> SCMapBakeComputePass::BakeMapTexture(const SCMapLoadingData& map_data)
	{

		if (map_data.megatile_width == 0 || map_data.megatile_height == 0)
		{
			ERR_MSG("Invalid map size. Width and height must be greater than 0.");
			return nullptr;
		}
		if (map_data.mtxm.size() < (map_data.megatile_width * map_data.megatile_height))
		{
			ERR_MSG("MTXM data is invalid.");
			return nullptr;
		}
		if ((size_t)map_data.terrain_type >= tileset_gpu_buffers_.size())
		{
			ERR_MSG("Invalid terrain type or TileSet GPU data is not created.");
			return nullptr;
		}

		const auto& tileset_gpu = tileset_gpu_buffers_[(size_t)map_data.terrain_type];
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
		map_info_cb.megatile_size = { map_data.megatile_width, map_data.megatile_height };
		
		if (map_info_buffer_ == nullptr)
		{
			map_info_buffer_ = EntityManager::CreateEntity<ConstantBuffer>();
			result = map_info_buffer_->Create<MapInfoCB>();
			if (!result) { return nullptr; }
		}
		map_info_buffer_->Upload(context, map_info_cb);

		// CB bind
		map_info_buffer_->Bind(context, ShaderStage::Flags::Compute, REG_B_MAP_INFO);

		s_ptr<Texture2D> map_texture = EntityManager::CreateEntity<Texture2D>();
		D3D11_TEXTURE2D_DESC desc = {};
		desc.Format = DXGI_FORMAT_R8_UINT;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		desc.Width = map_data.megatile_width * (UINT)megatile_size;
		desc.Height = map_data.megatile_height * (UINT)megatile_size;

		result = map_texture->CreateTexture2D(&desc, nullptr);
		if (!result) { return nullptr; }

		// Tileset Bind
		tileset_gpu.CV5_megatiles->BindSRV(context, ShaderStage::Flags::Compute, REG_T_TILESET_CV5);
		tileset_gpu.VX4_minitiles->BindSRV(context, ShaderStage::Flags::Compute, REG_T_TILESET_VX4);
		tileset_gpu.VR4_minitile_wpe_indices->BindSRV(context, ShaderStage::Flags::Compute, REG_T_TILESET_VR4);

		// scx 맵 데이터(mtxm) 생성 및 binding
		u_ptr<TypedBuffer> mtxm_buffer = EntityManager::CreateEntity<TypedBuffer>();
		std::span<const MTXM> mtxm_span = map_data.mtxm;
		result = mtxm_buffer->CreateImmutableBuffer(DXGI_FORMAT_R16_UINT, mtxm_span);
		if (!result) { return nullptr; }

		mtxm_buffer->BindSRV(context, ShaderStage::Flags::Compute, REG_T_MAP_MTXM);

		// Texture Bind
		map_texture->BindUAV(context, REG_U_MAP_TEXTURE);

		thread_count_ = { desc.Width, desc.Height, 1 };

		Execute(context);

		map_texture->UnbindUAV(context, REG_U_MAP_TEXTURE);

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