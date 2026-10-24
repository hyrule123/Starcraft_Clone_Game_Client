#include "Content/pch.h"
#include "SCResourceManager.h"

#include <Engine/Manager/ResourceManager.h>
#include <Engine/Manager/RenderManager.h>

#include <Engine/Core/Debug.h>

#include <Engine/Resource/GPU/Buffer/Texture2DArray.h>
#include <Engine/Resource/GPU/Buffer/TypedBuffer.h>
#include <Engine/Resource/GPU/Buffer/StructuredBuffer.h>
#include <Engine/Resource/GPU/PipelineState.h>
#include <Engine/Resource/GPU/Material.h>
#include <Engine/Resource/GPU/Mesh.h>
#include <Engine/Resource/GPU/State/RasterizerState.h>
#include <Engine/Resource/GPU/Shader/InputLayout.h>
#include <Engine/Resource/GPU/State/DepthStencilState.h>
#include <Engine/Resource/GPU/Shader/VertexShader.h>

#include <Engine/Resource/GPU/Vertex.h>

#include <Content/SCData/Constants.h>
#include <Content/SCData/MPQ/MPQFileSystem.h>
#include <Content/SCData/PCXFile.h>
#include <Content/SCData/SCImageLibrary.h>

#include <algorithm>

#include <Content/HLSL/SCImage/SCImageCommon.hlsli>

namespace
{
	constexpr uint32_t kWarpTextureImageType = 210;	// OpenBW ImageTypes::IMAGEID_Warp_Texture
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

		sc_map_loader_.Init(mpq_file_system_);

		CreateTunitBuffer(mpq_file_system_);
		CreateDepthStencilStates();
		CreateOriginalMaterials();
		CreateMapMaterial();
		materials_.resize(sc_image_library_.GrpCount());
		CreateMesh();
	}
	std::shared_ptr<engine::Material> SCResourceManager::GetNewMapMaterial() const
	{
		return map_material_->Clone();
	}

	void SCResourceManager::CreateTunitBuffer(const MPQFileSystem& mpq_fs)
	{
		using namespace engine;

		// TUnit
		auto tunit_data = mpq_fs.ReadFile(L"game\\tunit.pcx");
		CHECK(tunit_data.has_value());

		PcxFile tunit_pcx;
		bool result = tunit_pcx.Load(*tunit_data);
		CHECK(result);

		tunit_buffer_ = EntityManager::CreateEntity<TypedBuffer>();
		result = tunit_buffer_->CreateImmutableBuffer(DXGI_FORMAT_R8_UINT, tunit_pcx.pixels());
		CHECK(result);

		auto& per_frame_table = RenderManager::GetInst().GetPerFrameBindingTable();
		per_frame_table.AddShaderResource(ShaderStage::Vertex | ShaderStage::Pixel, REG_T_SC_TUNIT, tunit_buffer_);
	}

	void SCResourceManager::CreateDepthStencilStates()
	{
		using namespace engine;

		auto GetCommonDesc = []() -> D3D11_DEPTH_STENCIL_DESC
			{
				D3D11_DEPTH_STENCIL_DESC desc = {};
				desc.DepthEnable = TRUE;
				desc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;
				desc.StencilEnable = FALSE;
				return desc;
			};

		depth_key_write_dss_ = EntityManager::CreateEntity<DepthStencilState>();
		{
			D3D11_DEPTH_STENCIL_DESC desc = GetCommonDesc();

			desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;

			bool result = depth_key_write_dss_->Create(desc);
			CHECK_F(result, "Failed to create DepthStencilState");
		}

		depth_key_test_dss_ = EntityManager::CreateEntity<DepthStencilState>();
		{
			D3D11_DEPTH_STENCIL_DESC desc = GetCommonDesc();

			desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; // 깊이 버퍼에 쓰지 않음. (반투명 / 이펙트 처리)

			bool result = depth_key_test_dss_->Create(desc);
			CHECK_F(result, "Failed to create DepthStencilState");
		}
	}

	void SCResourceManager::CreateOriginalMaterials()
	{
		using namespace engine;

		std::shared_ptr<VertexShader> vs = ResourceManager::GetInst().LoadFromFile<VertexShader>("Shader/SCImage_VS.cso");

		std::array<D3D11_INPUT_ELEMENT_DESC, 2> desc = {};
		desc[0].SemanticName = "TEXCOORD";
		desc[0].SemanticIndex = 0;
		desc[0].Format = DXGI_FORMAT_R32G32_FLOAT;
		desc[0].InputSlot = 0;
		desc[0].AlignedByteOffset = 0;
		desc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		desc[0].InstanceDataStepRate = 0;

		desc[1] = vertex::instance_id::kInputLayoutDesc;

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


		// 공통: VS / InputLayout / RSS / 인스턴스 stride는 모든 이미지 파이프라인이 공유
		auto create_pipeline_state = [&](const HashedStringView& ps_name, const std::shared_ptr<DepthStencilState>& dss, DrawFlags draw_flags)
			{
				std::shared_ptr<PipelineState> state = EntityManager::CreateEntity<PipelineState>();
				state->SetInputLayout(il);
				state->SetVertexShader(vs);

				const bool ps_result = state->SetPixelShader(ps_name);
				CHECK_F(ps_result, "Failed to load pixel shader.");

				state->SetInstanceDataStride(sizeof(SCImageInstance));
				state->SetRasterizerState(rss);
				state->SetDepthStencilState(dss);
				state->SetDrawFlags(draw_flags);
				return state;
			};

		auto create_original_material = [&](SCImageVariant variant, render_pass::ID pass_id, std::shared_ptr<PipelineState> state)
			{
				std::shared_ptr<Material>& material = orig_materials_[(size_t)variant];
				ASSERT(material == nullptr);		// 중복 호출 방지

				material = EntityManager::CreateEntity<Material>();
				material->SetPipelineState(pass_id, std::move(state));
			};

#pragma region OPAQUE PIPELINE
		// 깊이 기록, 배칭 허용
		create_original_material(SCImageVariant::Opaque, kSCOpaquePassID,
			create_pipeline_state("Shader/SCImageOpaque_PS.cso", depth_key_write_dss_, DrawFlags{}));
#pragma endregion OPAQUE PIPELINE

#pragma region EFFECT PIPELINE
		// 그림자 / 클로킹 / 리매핑 / 워프 플래시 등 old를 읽는 효과. 깊이 테스트만, 배칭 허용
		create_original_material(SCImageVariant::Effect, kSCEffectPassID,
			create_pipeline_state("Shader/SCImageEffect_PS.cso", depth_key_test_dss_, DrawFlags{}));
#pragma endregion EFFECT PIPELINE

#pragma region DISTORTION PIPELINE
		// 서브메시 0: B(미러)에서 오른쪽 n픽셀을 읽어 A에만 기록
		// 같은 드로우 안의 다른 픽셀 읽기는 ROV가 순서를 보장하지 않으므로 배칭 금지
		create_original_material(SCImageVariant::Distortion, kSCEffectPassID,
			create_pipeline_state("Shader/SCImageDistortion_PS.cso", depth_key_test_dss_, DrawFlag::NoBatching));

		// 서브메시 1: Read가 A에 쓴 결과를 B에 반영. Read와 같은 커버리지·깊이 조건
		// VS가 GRP 텍스처 크기로 쿼드를 만들므로 GRP별 Material(Clone)이 필요
		create_original_material(SCImageVariant::DistortionSync, kSCEffectPassID,
			create_pipeline_state("Shader/SCImageDistortionSync_PS.cso", depth_key_test_dss_, DrawFlag::NoBatching));
#pragma endregion DISTORTION PIPELINE


#pragma region WARP TEXTURE BINDING
		std::shared_ptr<Texture2DArray> warp_texture = sc_image_library_.GetGrp(kWarpTextureImageType).texture;
		CHECK_F(warp_texture, "Warp texture(image 210) not found");
		RenderManager::GetInst().GetPerFrameBindingTable().AddShaderResource(
			ShaderStage::Pixel, REG_T_SC_WARP_TEXTURE, warp_texture);
#pragma endregion //WARP TEXTURE BINDING
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
		
		bool result = image_mesh_->CreateVertexBuffer<ImageVSInput>(vertices, local_bounds);
		CHECK_F(result, "Failed to create vertex buffer");
		constexpr uint16_t indices[6] = { 0, 1, 2, 0, 2, 3 };
		result = image_mesh_->CreateIndexBuffer(indices, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, { {0, 6}, {0, 6} });
		CHECK_F(result, "Failed to create index buffer");
	}

	void SCResourceManager::CreateMapMaterial()
	{
		using namespace engine;
		map_mesh_ = ResourceManager::GetInst().Find<Mesh>("Mesh_Standard2D_Rect");
		CHECK(map_mesh_);

		map_material_ = EntityManager::CreateEntity<Material>();

		std::shared_ptr<PipelineState> pipeline_state = EntityManager::GetInst().CreateEntity<PipelineState>();
		pipeline_state->SetInstanceDataStride(sizeof(MapInfoCB));
		pipeline_state->SetVertexShader("Shader/SCMapRender_VS.cso");
		pipeline_state->SetPixelShader("Shader/SCMapRender_PS.cso");

		pipeline_state->SetDepthStencilState(depth_key_write_dss_);
		pipeline_state->SetRasterizerState("RSS_Solid_Back");
		pipeline_state->SetInputLayout("InputLayout_Standard2D");
		map_material_->SetPipelineState(sc::kSCOpaquePassID, pipeline_state);
	}

	std::shared_ptr<engine::Material> SCResourceManager::GetImageMaterial(int32_t image_type, SCImageVariant variant)
	{
		using namespace engine;

		ASSERT(variant < SCImageVariant::kCount);

		const sc::ImageDef& image_def = sc_image_library_.GetImageDef(image_type);

		if (image_def.grp_index >= materials_.size())
		{
			materials_.resize(image_def.grp_index + 1);
		}

		std::shared_ptr<Material>& mtrl = materials_[image_def.grp_index][(size_t)variant];
		if (mtrl == nullptr)
		{
			mtrl = orig_materials_[(size_t)variant]->Clone();
			BakedGrp grp = sc_image_library_.GetGrp(image_type);
			ASSERT_F(grp.texture && grp.frame_rects, "Failed to get texture for image type: {}", image_type);

			GPUBufferBindingTable& binding_table = mtrl->GetPerMaterialBindingTable();
			binding_table.AddShaderResource(engine::ShaderStage::Vertex | engine::ShaderStage::Pixel, REG_T_SC_GRP_SPRITE, std::move(grp.texture));
			binding_table.AddShaderResource(engine::ShaderStage::Vertex | engine::ShaderStage::Pixel, REG_T_SC_GRP_FRAME_RECTS, std::move(grp.frame_rects));
		}

		return mtrl;
	}
}

