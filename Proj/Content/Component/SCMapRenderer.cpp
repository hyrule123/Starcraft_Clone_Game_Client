#include "Content/pch.h"
#include "SCMapRenderer.h"

#include <Content/SCData/Constants.h>
#include <Content/SCData/SCResourceManager.h>

#include <Content/HLSL/SCMap/SCMapRender.hlsli>

#include <SCSimulator/Include/SCSimulatorTypes.h>

#include <Engine/Core/Debug.h>

#include <Engine/Resource/GPU/Material.h>
#include <Engine/Resource/GPU/Buffer/TypedBuffer.h>
#include <Engine/Resource/GPU/Buffer/Texture2DArray.h>

#include <Engine/Manager/RenderManager.h>
#include <Engine/Manager/GraphicsDevice.h>

#include <Engine/Game/Component/Transform.h>
#include <Engine/Game/Component/Camera.h>

namespace sc
{
	// g_tile_states 비트 배치 계약 (SCMapRender.hlsli)
	static_assert(sizeof(SCSim::TileData) == sizeof(uint32_t));
	static_assert(offsetof(SCSim::TileData, visible) == 0);
	static_assert(offsetof(SCSim::TileData, explored) == 1);
	static_assert(offsetof(SCSim::TileData, flags) * 8 == kTileStateFlagsShift);

	SCMapRenderer::SCMapRenderer()
		: Super(SCMapRenderer::kClassConcreteName)
	{}
	SCMapRenderer::~SCMapRenderer()
	{}
	void SCMapRenderer::Init()
	{
		Super::Init();

		using namespace engine;

		SetMesh(SCResourceManager::GetInst().GetMapMesh());
		SetMaterial(0, SCResourceManager::GetInst().GetNewMapMaterial());

		SetEnable(false);
	}
	void SCMapRenderer::LateUpdate()
	{
		using namespace engine;

		Super::LateUpdate();

		if (sc_map_ == nullptr)
		{
			DEBUG_LOG("SCMap is not set.");
			return;
		}

		if (!IsRenderReady())
		{
			DEBUG_LOG("SCMapRenderer is not ready to render. Material or Mesh is missing.");
			return;
		}

		if (needs_scale_matching_)
		{
			// SC 좌표계: 왼쪽 위가 (0,0), 오른쪽 아래가 (width, height) -> +x right, +y down
			// 원점을 기준으로 2사분면을 사용한다.
			Transform* tr = GetTransform();
			const float width = (float)sc_map_->megatile_width * kMegatilePixelSize;
			const float height = (float)sc_map_->megatile_height * kMegatilePixelSize;
			const float2 center = float2(width * 0.5f, - (height * 0.5f));

			Camera* cam = RenderManager::GetInst().GetMainCamera().get();
			CHECK(cam);

			const auto& desc = cam->GetProjectionMatrixDesc();
			float3 cam_pos = cam->GetComponent<Transform>()->GetLocalPosition();

			// 카메라를 맵의 중앙으로 이동
			cam_pos.x = center.x;
			cam_pos.y = center.y;
			cam->GetComponent<Transform>()->SetLocalPosition(cam_pos);

			// 맵을 카메라로부터 제일 멀리 떨어뜨려서 배치
			const float farthest_z = desc.far_z + cam_pos.z - 0.01f;
			tr->SetLocalPosition(float3(width * 0.5f, - (height * 0.5f), farthest_z));
			tr->SetLocalScale(float3(width, height, 1.0f));
			
			needs_scale_matching_ = false;
		}
	}
	void SCMapRenderer::OnDestroy()
	{
		using namespace engine;

		Super::OnDestroy();
		auto& per_frame_table = RenderManager::GetInst().GetPerFrameBindingTable();

		per_frame_table.RemoveShaderResource(REG_T_SC_WPE_PALETTE);
		per_frame_table.RemoveShaderResource(REG_T_SC_PCX_TILESET_LUT);
		per_frame_table.RemoveShaderResource(REG_T_SC_PCX_TILESET_LUT_OFFSETS);
		per_frame_table.RemoveShaderResource(REG_T_SC_CLOAK_FADE_SELECTOR);
	}
	void SCMapRenderer::WriteInstanceData(const engine::Renderer::InstanceWriteArgs& args)
	{
		using namespace engine;

		switch (args.pass)
		{
		case kSCOpaquePassID:
		{
			MapInfoCB map_info_cb = {};
			map_info_cb.megatile_size = { (int32_t)sc_map_->megatile_width, (int32_t)sc_map_->megatile_height };
			map_info_cb.world_mat = GetTransform()->GetWorldMatrix();

			args.block.Write(map_info_cb);
			break;
		}

		default:
		{
			ASSERT_F(false, "처리되지 않은 Pass ID: {}", static_cast<uint32_t>(args.pass));
			break;
		}

		}

		//if (sc_map_ == nullptr)
		//{
		//	ASSERT(false);
		//	return;
		//}
		//if(false == data_block.IsValid())
		//{
		//	ASSERT(false);
		//	return;
		//}

		//auto* context = GraphicsDevice::GetInst().GetContext();

		////TODO: 임시방편, Shader Set 단위로 묶는 함수가 필요할 듯
		//sc_map_->wpe_color_palettes->BindSRV(context, REG_T_SC_WPE_PALETTE, ShaderStage::kPS);

		//MapInfoCB map_info_cb = {};
		//map_info_cb.megatile_size = { sc_map_->megatile_width, sc_map_->megatile_height };

		//data_block.Write(map_info_cb);
	}
	void SCMapRenderer::SetSCMap(std::unique_ptr<SCMap> sc_map)
	{
		using namespace engine;

		if (sc_map == nullptr)
		{
			SetEnable(false);
			return;
		}

		sc_map_ = std::move(sc_map);

		Material* mtrl = GetMaterial(0);
		ASSERT(mtrl);
		
		mtrl->SetTexture(ShaderStage::Pixel, REG_T_MAP_TEXTURE, sc_map_->texture);

		// Creep / 타일셋 리소스 (맵 수명 동안 고정)
		mtrl->SetTexture(ShaderStage::Pixel, REG_T_CREEP_RANDOM_TABLE, sc_map_->creep_random_tiles);
		mtrl->SetTexture(ShaderStage::Pixel, REG_T_CREEP_EDGE_TEXTURE, sc_map_->creep_edge);

		// PER MATERIAL
		GPUBufferBindingTable& mtrl_table = mtrl->GetPerMaterialBindingTable();
		mtrl_table.AddShaderResource(ShaderStage::Pixel, REG_T_CV5, sc_map_->cv5_megatiles);
		mtrl_table.AddShaderResource(ShaderStage::Pixel, REG_T_VX4, sc_map_->vx4_minitiles);
		mtrl_table.AddShaderResource(ShaderStage::Pixel, REG_T_VR4, sc_map_->vr4_minitile_wpe_indices);

		// 타일 상태 (버퍼 객체는 고정, 내용은 ApplyTileSnapshot에서 매 프레임 갱신)
		const uint32_t tile_count = sc_map_->megatile_width * sc_map_->megatile_height;

		tile_states_ = EntityManager::CreateEntity<TypedBuffer>();
		static_assert(sizeof(SCSim::TileData) == GetDXGIFormatByteStride(DXGI_FORMAT_R32_UINT), "TileData size does not match DXGI_FORMAT_R32_UINT");
		CHECK_F(tile_states_->CreateDynamicBuffer(DXGI_FORMAT_R32_UINT, tile_count), "Failed to create tile state buffer");
		mtrl_table.AddShaderResource(ShaderStage::Pixel, REG_T_TILE_STATES, tile_states_);


		// PER FRAME
		auto& per_frame_table = RenderManager::GetInst().GetPerFrameBindingTable();
		per_frame_table.AddShaderResource(ShaderStage::Vertex | ShaderStage::Pixel, REG_T_SC_WPE_PALETTE, sc_map_->wpe_color_palettes);

		per_frame_table.AddShaderResource(ShaderStage::Pixel, REG_T_SC_PCX_TILESET_LUT, sc_map_->pcx_tileset_LUT);

		per_frame_table.AddShaderResource(ShaderStage::Pixel, REG_T_SC_PCX_TILESET_LUT_OFFSETS, sc_map_->pcx_tileset_LUT_offsets);

		per_frame_table.AddShaderResource(ShaderStage::Pixel, REG_T_SC_CLOAK_FADE_SELECTOR, sc_map_->cloak_fade_selector);

		needs_scale_matching_ = true;
		SetEnable(true);
	}

	bool SCMapRenderer::ApplyTileSnapshot(const SCSim::TileData* tiles, uint32_t width, uint32_t height)
	{
		using namespace engine;

		ASSERT(tiles != nullptr);
		if (sc_map_ == nullptr || tile_states_ == nullptr)
		{
			return false;
		}

		// SCSim과 Content 맵 로더가 같은 맵을 같은 크기로 해석했는지 확인
		if (width != sc_map_->megatile_width || height != sc_map_->megatile_height)
		{
			ERR_MSG("Tile snapshot size mismatch: SCSim {}x{}, SCMap {}x{}", width, height, sc_map_->megatile_width, sc_map_->megatile_height);
			return false;
		}

		return tile_states_->Upload(GraphicsDevice::GetInst().GetContext(), tiles, static_cast<uint32_t>(sizeof(SCSim::TileData)), width * height);
	}
	
}