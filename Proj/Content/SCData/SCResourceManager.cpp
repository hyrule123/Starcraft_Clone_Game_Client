#include "Content/pch.h"
#include "SCResourceManager.h"

#include <Engine/Manager/ResourceManager.h>
#include <Engine/Manager/RenderManager.h>

#include <Engine/Core/Debug.h>

#include <Engine/Resource/GPU/Buffer/Texture2DArray.h>
#include <Engine/Resource/GPU/Buffer/TypedBuffer.h>
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
		materials_.resize(sc_image_library_.grp_count());
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

#pragma region NORMAL PIPELINE
		std::shared_ptr<Material>& normal_material = orig_materials_[(size_t)SCImageVariant::Opaque];
		ASSERT(normal_material == nullptr);		// 중복 호출 방지
		normal_material = EntityManager::CreateEntity<Material>();
		
		std::shared_ptr<PipelineState> normal_state = EntityManager::CreateEntity<PipelineState>();
		normal_state->SetInputLayout(il);
		normal_state->SetVertexShader(vs);
		normal_state->SetPixelShader("Shader/SCImageOpaque_PS.cso");

		normal_state->SetInstanceDataStride(sizeof(SCImageInstance));
		normal_state->SetDepthStencilState(depth_key_write_dss_);
		normal_state->SetRasterizerState(rss);

		normal_material->SetPipelineState(sc::kSCOpaquePassID, normal_state);
#pragma endregion NORMAL PIPELINE

#pragma region SHADOW PIPELINE
		std::shared_ptr<Material>& shadow_material = orig_materials_[(size_t)SCImageVariant::Effect];
		ASSERT(shadow_material == nullptr);		// 중복 호출 방지
		shadow_material = EntityManager::CreateEntity<Material>();

		std::shared_ptr<PipelineState> shadow_state = EntityManager::CreateEntity<PipelineState>();
		shadow_state->SetInputLayout(il);
		shadow_state->SetVertexShader(vs);
		shadow_state->SetPixelShader("Shader/SCImageEffect_PS.cso");
		shadow_state->SetInstanceDataStride(sizeof(SCImageInstance));
		shadow_state->SetRasterizerState(rss);
		shadow_state->SetDepthStencilState(depth_key_test_dss_);

		shadow_material->SetPipelineState(kSCEffectPassID, shadow_state);
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

	std::shared_ptr<engine::Material> SCResourceManager::GetImageMaterial(uint32_t image_type, SCImageVariant variant)
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
			std::shared_ptr<Texture2D> tex = sc_image_library_.GetTexture(image_type);
			ASSERT_F(tex, "Failed to get texture for image type: {}", image_type);
			mtrl->SetTexture(engine::ShaderStage::Vertex | engine::ShaderStage::Pixel, REG_T_SC_GRP_SPRITE, tex);
		}

		return mtrl;
	}
}

