#include "Content/pch.h"
#include "SCResourceManager.h"

#include <Engine/Manager/ResourceManager.h>
#include <Engine/Manager/RenderManager.h>

#include <Engine/Core/Debug.h>

#include <Engine/Util/file_system.h>

#include <Engine/Resource/GPU/Buffer/Texture2DArray.h>
#include <Engine/Resource/GPU/Buffer/StructuredBuffer.h>
#include <Engine/Resource/GPU/Buffer/TypedBuffer.h>
#include <Engine/Resource/GPU/PipelineState.h>
#include <Engine/Resource/GPU/Material.h>
#include <Engine/Resource/GPU/Mesh.h>
#include <Engine/Resource/GPU/State/RasterizerState.h>
#include <Engine/Resource/GPU/Shader/InputLayout.h>
#include <Engine/Resource/GPU/State/DepthStencilState.h>
#include <Engine/Resource/GPU/State/BlendState.h>
#include <Engine/Resource/GPU/Shader/VertexShader.h>
#include <Engine/Resource/GPU/Shader/InputLayout.h>

#include <Content/SCData/MPQ/MPQArchive.h>
#include <Content/SCData/MPQ/MPQFileSystem.h>
#include <Content/SCData/SCMapBakeComputePass.h>
#include <Content/SCData/SCTileSetPcxLUT.h>
#include <Content/SCData/PCXFile.h>
#include <Content/SCData/SCImageLibrary.h>

#include <fstream>
#include <iterator>
#include <algorithm>

#include <Content/HLSL/SCMapRender.hlsli>
#include <Content/HLSL/SCImage/SCImageCommon.hlsli>

#include <Engine/Util/file_system.h>
#include <Engine/Resource/GPU/Shader/PixelShader.h>

namespace
{
	constexpr uint8_t kModifierShadow = 10;

	sc::SCImageMaterialVariant GetMaterialVariant(uint8_t modifier)
	{
		return (modifier == kModifierShadow) ? sc::SCImageMaterialVariant::Shadow : sc::SCImageMaterialVariant::Normal;
	}
}

namespace sc
{
	SCResourceManager::SCResourceManager()
	{
	}

	SCResourceManager::~SCResourceManager()
	{}
	void SCResourceManager::Init()
	{
		using namespace engine;

		CHECK_F(has_initialized_ == false, "Double initialization");

		has_initialized_ = true;

		mpq_dir_ = ResourceManager::GetInst().GetResourceDir() / "SCSim" / "mpq";

		mpq_file_system_.AddArchive(mpq_dir_ / "patch_rt.mpq", true);
		mpq_file_system_.AddArchive(mpq_dir_ / "BrooDat.mpq", true);
		mpq_file_system_.AddArchive(mpq_dir_ / "stardat.mpq", true);

		bool result = sc_image_library_.Init(mpq_file_system_);
		CHECK_F(result, "Failed to initialize SCImageLibrary");

		// CPU
		result = LoadTilesetData(mpq_file_system_);
		CHECK_F(result, "Failed to load TileSet data");

		// GPU
		result = CreateTileSetGPUData(tilesets_);
		CHECK_F(result, "Failed to create TileSet GPU data");
		sc_map_baker_ = EntityManager::CreateEntity<SCMapBakeComputePass>();

		CreateOriginalMaterials();
		materials_.resize(sc_image_library_.grp_count());
		CreateMesh();
	}
	bool SCResourceManager::LoadTilesetData(const MPQFileSystem& mpq_file_system)
	{
		CHECK(has_initialized_);

		using namespace engine;

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

		// TUnit
		auto tunit_data = mpq_file_system.ReadFile(L"game\\tunit.pcx");
		CHECK(tunit_data.has_value());

		PcxFile tunit_pcx;
		bool result = tunit_pcx.Load(*tunit_data);
		CHECK(result);

		const auto& pixels = tunit_pcx.pixels();

		tunit_buffer_ = EntityManager::CreateEntity<TypedBuffer>();
		result = tunit_buffer_->CreateImmutableBuffer(DXGI_FORMAT_R8_UINT, tunit_pcx.pixels());
		CHECK(result);

		auto& per_frame_table = RenderManager::GetInst().GetPerFrameBindingTable();
		per_frame_table.AddShaderResource(ShaderStage::Vertex | ShaderStage::Pixel, REG_T_SC_TUNIT, tunit_buffer_);

		return true;
	}

	bool SCResourceManager::CreateTileSetGPUData(const std::array<TileSetData, (size_t)TileSetType::kCount>& tileset_data)
	{
		using namespace engine;

		auto clear_tilesets = [&]() {
			for (size_t i = 0; i < tileset_gpu_buffers_.size(); ++i)
			{
				tileset_gpu_buffers_[i] = TileSetDataGPU();
			}
			};

		std::vector<uint16> megatile_data;
		bool result = false;

		//index tileset
		for (size_t tileset_i = 0; tileset_i < tileset_gpu_buffers_.size(); ++tileset_i)
		{
			const TileSetData& tileset = tileset_data[tileset_i];
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

#pragma region PCX - Tileset LUT
			{
				auto& pcx_lut = tileset.pcx_lut;
				auto& pcx_lut_gpu = tileset_gpu.PCX_tileset_LUT;
				pcx_lut_gpu = EntityManager::CreateEntity<TypedBuffer>();

				std::span<const sc::SCTileSetPcxLUT::ElemType> pcx_lut_span = pcx_lut->data();
				result = pcx_lut_gpu->CreateImmutableBuffer(DXGI_FORMAT_R8_UINT, pcx_lut_span);
				CHECK(result);
			}
#pragma endregion PCX - Tileset LUT

		}

		return true;
	}

	std::unique_ptr<SCMap> SCResourceManager::LoadMapDataCPU(const std::filesystem::path& map_path)
	{
		using namespace engine;

		ASSERT(sc_map_baker_ != nullptr);

		SCMapLoadingData map_data = LoadMapLoadingData(map_path);

		// 지형 타입에 맞는 타일셋
		TileSetData& tileset = tilesets_[(size_t)map_data.terrain_type];

		// 픽셀 색 정보 컨테이너 할당
		size_t pixel_width = (size_t)map_data.megatile_width * (size_t)32;
		size_t pixel_height = (size_t)map_data.megatile_height * (size_t)32;
		std::vector<uint8> WPE_idx;
		WPE_idx.resize(pixel_width * pixel_height);

		//채워넣기
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

							// 색상을 가져오지 않고, WPE 색인만 저장하도록 변경했음.
							//const WPE& c = tileset->wpe[idx];
							//RGBA& pixel = pixels[y * pixel_width + x];
							//pixel.r = c.r;
							//pixel.g = c.g;
							//pixel.b = c.b;
							//pixel.a = 0xFF;	 // 알파 채널은 항상 255로 설정(원래 없음)
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
		bool result = result_map->texture->CreateTexture2D(&desc, &init_data);
		if (result == false) { return nullptr; }

		result_map->megatile_width = map_data.megatile_width;
		result_map->megatile_height = map_data.megatile_height;
		result_map->terrain_type = map_data.terrain_type;
		result_map->unit_data = std::move(map_data.unit_data);
		result_map->wpe_color_palettes = GetWPEColorPaletteBuffer(map_data.terrain_type);
		result_map->pcx_tileset_LUT = GetPCXTilesetLUTBuffer(map_data.terrain_type);

		return result_map;
	}
	std::unique_ptr<SCMap> SCResourceManager::LoadMapDataGPU(const std::filesystem::path& map_path)
	{

		SCMapLoadingData map_data = LoadMapLoadingData(map_path);

		std::unique_ptr<SCMap> result_map = std::make_unique<SCMap>();
		result_map->texture = sc_map_baker_->BakeMapTexture(map_data);
		if(result_map->texture == nullptr) { return nullptr; }
		
		result_map->megatile_width = map_data.megatile_width;
		result_map->megatile_height = map_data.megatile_height;
		result_map->terrain_type = map_data.terrain_type;
		result_map->unit_data = std::move(map_data.unit_data);
		result_map->wpe_color_palettes = GetWPEColorPaletteBuffer(map_data.terrain_type);
		result_map->pcx_tileset_LUT = GetPCXTilesetLUTBuffer(map_data.terrain_type);

		return result_map;
	}

	SCMapLoadingData SCResourceManager::LoadMapLoadingData(const std::filesystem::path& map_path)
	{
		MPQArchive mpq_archive;
		bool result = mpq_archive.OpenMPQFile(map_path);

		if (false == result)
		{
			ERR_MSG("Failed to open MPQ archive.");
			return SCMapLoadingData{};
		}

		SCMapLoadingData map_data = {};

		MPQFile map_file = mpq_archive.OpenInnerFile("staredit\\scenario.chk");
		if (map_file.ReadAll(map_data.map_raw_bytes) == false)
		{
			ERR_MSG("Failed to read scenario.chk file.");
			return SCMapLoadingData{};
		}

		map_data.map_data_table = ParseMapData(map_data.map_raw_bytes);

		// 맵 사이즈
		auto megatile_count = GetMegaTileCount(map_data.map_data_table);
		const uint32& megatile_width = megatile_count[0];
		const uint32& megatile_height = megatile_count[1];
		if (megatile_width == 0 || megatile_height == 0) { return SCMapLoadingData{}; }
		map_data.megatile_width = megatile_width;
		map_data.megatile_height = megatile_height;

		// 지형 타입
		auto terrain_type = GetTerrainType(map_data.map_data_table);
		if (terrain_type == TileSetType::kCount) { return SCMapLoadingData{}; }
		map_data.terrain_type = terrain_type;

		// MTXM 데이터
		map_data.mtxm = GetMTXM(map_data.map_data_table, megatile_width, megatile_height);
		if (map_data.mtxm.empty()) { return SCMapLoadingData{}; }

		// Unit Data (비어있어도 무관)
		map_data.unit_data = GetUnitData(map_data.map_data_table);

		map_data.tileset = &tilesets_[(size_t)terrain_type];
		map_data.tileset_gpu = &tileset_gpu_buffers_[(size_t)terrain_type];

		return map_data;
	}

	void SCResourceManager::CreateOriginalMaterials()
	{
		using namespace engine;

		std::shared_ptr<VertexShader> vs = ResourceManager::GetInst().LoadFromFile<VertexShader>("Shader/SCImage_VS.cso");
		D3D11_INPUT_ELEMENT_DESC desc[1] = {};
		desc[0].SemanticName = "TEXCOORD";
		desc[0].SemanticIndex = 0;
		desc[0].Format = DXGI_FORMAT_R32G32_FLOAT;
		desc[0].InputSlot = 0;
		desc[0].AlignedByteOffset = 0;
		desc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		desc[0].InstanceDataStepRate = 0;
		std::shared_ptr<InputLayout> il = EntityManager::CreateEntity<InputLayout>();
		bool result = il->Create({ desc }, vs.get());
		CHECK_F(result, "Failed to create InputLayout.");

		std::shared_ptr<RasterizerState> rss = EntityManager::CreateEntity<RasterizerState>();
		D3D11_RASTERIZER_DESC rsDesc = {};
		rsDesc.FillMode = D3D11_FILL_SOLID;			// 선으로만 그림
		rsDesc.CullMode = D3D11_CULL_NONE;			// 백페이스 컬링 X
		rsDesc.FrontCounterClockwise = false;
		rsDesc.DepthClipEnable = false;				//Depth Clip OFF
		if (!rss->Create(rsDesc)) { CHECK(false); }

#pragma region NORMAL PIPELINE
		std::shared_ptr<Material>& normal_material = orig_materials_[(size_t)SCImageMaterialVariant::Normal];
		ASSERT(normal_material == nullptr);		// 중복 호출 방지
		normal_material = EntityManager::CreateEntity<Material>();
		
		std::shared_ptr<PipelineState> normal_state = EntityManager::CreateEntity<PipelineState>();
		normal_state->SetInputLayout(il);
		normal_state->SetVertexShader(vs);
		normal_state->SetPixelShader("Shader/SCImageOpaque_PS.cso");

		normal_state->SetPerInstanceDataStride(sizeof(SCImageInstance));
		normal_state->SetInstancingSupport(true);
		normal_state->SetDepthStencilState("DSS_Default");
		normal_state->SetRasterizerState(rss);

		normal_material->SetPipelineState(RenderPassOrder::ForwardOpaque, normal_state);
#pragma endregion NORMAL PIPELINE

#pragma region SHADOW PIPELINE
		std::shared_ptr<Material>& shadow_material = orig_materials_[(size_t)SCImageMaterialVariant::Shadow];
		ASSERT(shadow_material == nullptr);		// 중복 호출 방지
		shadow_material = EntityManager::CreateEntity<Material>();

		std::shared_ptr<PipelineState> shadow_state = EntityManager::CreateEntity<PipelineState>();
		shadow_state->SetInputLayout(il);
		shadow_state->SetVertexShader(vs);
		shadow_state->SetPixelShader("Shader/SCImageShadow_PS.cso");
		shadow_state->SetPerInstanceDataStride(sizeof(SCImageInstance));
		shadow_state->SetInstancingSupport(true);
		shadow_state->SetRasterizerState(rss);

		// Check ROV support
		{
			ComPtr<ID3D11PixelShader> ps = shadow_state->GetPixelShader()->GetD3D11PixelShader();

			ComPtr<ID3D11ShaderReflection> reflection;

			auto pth = ResourceManager::GetInst().GetResourceDir();
			pth /= "Shader/SCImageShadow_PS.cso";

			auto shader_bytecode = engine::file_system::ReadAllBytes(pth);

			HRESULT hr = D3DReflect(
				shader_bytecode.data(),
				shader_bytecode.size(),
				IID_ID3D11ShaderReflection,
				reinterpret_cast<void**>(reflection.GetAddressOf())
			);

			bool rov_supported = reflection->GetRequiresFlags() & D3D_SHADER_REQUIRES_ROVS;
			CHECK(rov_supported);
		}
		
		std::shared_ptr<DepthStencilState> dss = EntityManager::CreateEntity<DepthStencilState>();
		{
			D3D11_DEPTH_STENCIL_DESC desc = {};
			// ------------------------------------------------------------------
			// 1. 깊이(Depth) 테스트 설정(ON + 쓰기 off)
			// ------------------------------------------------------------------
			desc.DepthEnable = TRUE;                           // 깊이 테스트 ON.
			desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; // 깊이 버퍼에 쓰지 않음. (반투명 그림자만 그리므로)
			desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;                       // 깊이 테스트는 LESS로 설정. (기본값)

			// ------------------------------------------------------------------
			// 2. 스텐실(Stencil) 테스트 설정(OFF)
			// ------------------------------------------------------------------
			desc.StencilEnable = FALSE;                        // 스텐실 테스트를 끕니다.
			result = dss->Create(desc);
			CHECK_F(result, "Failed to create DepthStencilState");
		}
		shadow_state->SetDepthStencilState(dss);

		std::shared_ptr<BlendState> multiply_bs = EntityManager::CreateEntity<BlendState>();
		{
			D3D11_BLEND_DESC desc = {};
			desc.RenderTarget[0].BlendEnable = TRUE;
			// 결과 = 그림자 출력 × 기존 화면색
			// 0.5 넣으면 기존 색의 0.5배를 곱함(어두워짐)
			desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ZERO;
			desc.RenderTarget[0].DestBlend = D3D11_BLEND_SRC_COLOR;
			desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
			desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
			desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			result = multiply_bs->Create(desc);
			CHECK_F(result, "Failed to create multiply BlendState");
		}
		shadow_state->SetBlendState(multiply_bs);
		
		shadow_material->SetPipelineState(RenderPassOrder::ForwardBlend, shadow_state);
#pragma endregion SHADOW PIPELINE
	}

	void SCResourceManager::CreateMesh()
	{
		using namespace engine;
		image_mesh_ = EntityManager::CreateEntity<engine::Mesh>();

		constexpr ImageVSInput vertices[4] = 
		{
			{ float2(0.0f, 0.0f) },
			{ float2(1.0f, 0.0f) },
			{ float2(1.0f, 1.0f) },
			{ float2(0.0f, 1.0f)}
		};

		constexpr AABB3D local_bounds = 
		{
			.min = float3(-0.5f, -0.5f, -0.5f),
			.max = float3(0.5f, 0.5f, 0.5f)
		};
		
		bool result = image_mesh_->CreateVertexBuffer<ImageVSInput>(vertices, local_bounds); // YES
		CHECK_F(result, "Failed to create vertex buffer");

		constexpr uint16 indices[6] = { 0, 1, 2, 0, 2, 3 };
		result = image_mesh_->CreateIndexBuffer(indices, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		CHECK_F(result, "Failed to create index buffer");
	}

	engine::StringHashTable<std::vector<uint8>> SCResourceManager::ParseMapData(const std::vector<uint8>& map_data)
	{
		using namespace engine;

		// ERA, DIM, MTXM, UNIT 파일을 순차적으로 읽어와서 처리
		StringHashTable <std::vector<uint8>> map_data_table;

		int64 cursor = 0;
		
		constexpr size_t kMaxIterations = 1000000; // 안전장치: 무한루프 방지
		for(size_t i = 0; i < kMaxIterations; ++i)
		{
			if (cursor + 8 > (int64)map_data.size())
			{
				break;
			}

			// type_name == 4바이트
			std::string_view type_name(reinterpret_cast<const char*>(&map_data[cursor]), ChunkIDByteStride);

			// uint32 형태로 비교 가능하나, 타입명을 확인하기 어려워서 일단은 string_view로 처리
			uint32 chunk_ID = ChunkID(type_name);

			// data length는 음수가 될 수 있다. (커서를 앞으로 보냄)
			int32  data_length;
			memcpy(&data_length, &map_data[cursor + ChunkIDByteStride], sizeof(data_length));
			if (data_length < 0)
			{
				ERR_MSG("Invalid data length. Map may be protected. Please check.");
				break;
			}

			int64 next_cursor = cursor + (int64)ChunkIDByteStride * 2ull + (int64)data_length;

			if (0 <= next_cursor && next_cursor <= (int64)map_data.size())
			{
				auto it = map_data_table.find(type_name);
				if (it != map_data_table.end())
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
	std::array<uint32, 2> SCResourceManager::GetMegaTileCount(const engine::StringHashTable<std::vector<uint8>>& map_data_table)
	{
		using namespace engine;

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

		return {(uint32)width, (uint32)height};
	}

	// return TileSetType::kEND if the map is invalid
	TileSetType SCResourceManager::GetTerrainType(const engine::StringHashTable<std::vector<uint8>>& map_data_table)
	{
		static_assert((int32)TileSetType::kCount == 8, "TileSetType::kEND must be 8.");

		auto it = map_data_table.find(SCChunkTypeName[(uint32)SCChunkType::kTerrain]);

		if (it == map_data_table.end())
		{
			ERR_MSG("ERA chunk not found in map data.");
			return TileSetType::kCount;
		}

		const std::vector<uint8>& era_data = it->second;
		if(era_data.size() < 2)
		{
			ERR_MSG("ERA chunk data must at least 2 bytes.");
			return TileSetType::kCount;
		}

		//첫 1바이트에 지형 정보가 들어있다.
		uint8 terrain_byte = era_data[0];

		//앞의 5비트는 0으로 변환 (유효 비트는 하위 3비트: 0~7)
		constexpr uint8 terrain_mask = 0b00000111;
		terrain_byte &= terrain_mask;

		return static_cast<TileSetType>(terrain_byte);
	}

	std::vector<UnitData> SCResourceManager::GetUnitData(const engine::StringHashTable<std::vector<uint8>>& map_data_table)
	{
		auto it = map_data_table.find(SCChunkTypeName[(uint32)SCChunkType::kUnitPlacement]);

		if (it == map_data_table.end())
		{
			ERR_MSG("UNIT chunk not found in map data.");
			return std::vector<UnitData>();
		}

		const std::vector<uint8>& unit_data_chunk = it->second;

		// UnitData 구조체는 36바이트 단위
		size_t num_data = unit_data_chunk.size() / sizeof(UnitData);

		// 저장된 데이터를 옮긴다.
		std::vector<UnitData> unit_data_vector;
		if (num_data > 0)
		{
			unit_data_vector.resize(num_data);
			memcpy(unit_data_vector.data(), unit_data_chunk.data(), sizeof(UnitData) * num_data);
		}

		return unit_data_vector;
	}
	std::vector<MTXM> SCResourceManager::GetMTXM(const engine::StringHashTable<std::vector<uint8>>& map_data_table, uint32 megatile_width, uint32 megatile_height)
	{
		auto it = map_data_table.find(SCChunkTypeName[(uint32)SCChunkType::kTileMapAtlas]);
		if (it == map_data_table.end())
		{
			ERR_MSG("MTXM chunk not found in map data.");
			return {};
		}

		const std::vector<uint8>& mtxm_data = it->second;
		
		std::vector<MTXM> mtxms((size_t)megatile_width * (size_t)megatile_height);

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
		// 데이터 사이즈가 모자라도 정상 동작이라고 함(null tile)
		size_t copy_size = std::min(mtxm_data.size(), mtxms.size() * sizeof(MTXM));

		memcpy(mtxms.data(), mtxm_data.data(), copy_size);

		return mtxms;
	}
	std::shared_ptr<engine::Material> SCResourceManager::GetImageMaterial(uint32 image_type, SCImageMaterialVariant variant)
	{
		using namespace engine;

		ASSERT(variant < SCImageMaterialVariant::kCount);

		const sc::ImageDef& image_def = sc_image_library_.GetImageDef(image_type);

		if (image_def.grp_index >= materials_.size())
		{
			materials_.resize(image_def.grp_index + 1);
		}

		std::shared_ptr<Material>& mtrl = materials_[image_def.grp_index][(size_t)variant];
		if (mtrl == nullptr)
		{
			mtrl = orig_materials_[(size_t)variant]->Clone();
			std::shared_ptr<Texture2D> tex = sc_image_library_.GetTexture(image_type);
			ASSERT_F(tex, "Failed to get texture for image type: {}", image_type);
			mtrl->SetTexture(engine::ShaderStage::Vertex | engine::ShaderStage::Pixel, REG_T_SC_GRP_SPRITE, tex);
		}

		return mtrl;
	}
}

