#include "Content/pch.h"
#include "MainGameScene.h"

#include <Engine/Manager/GraphicsDevice.h>
#include <Engine/Manager/ResourceManager.h>
#include <Engine/Manager/RenderManager.h>
#include <Engine/Manager/TimeManager.h>

#include <Engine/Resource/GPU/RenderTargetGroup.h>
#include <Engine/Resource/GPU/Buffer/RenderTarget.h>
#include <Engine/Resource/GPU/Buffer/DepthStencilTarget.h>
#include <Engine/Resource/GPU/Buffer/ConstantBuffer.h>
#include <Engine/Resource/GPU/Shader/PixelShader.h>

#include <Engine/Render/RenderPass/MeshRenderPass.h>
#include <Engine/Render/RenderPass/BlitPass.h>

#include <Engine/Game/GameObject.h>
#include <Engine/Game/Component/Camera.h>
#include <Engine/Game/Component/Transform.h>

#include <Engine/Core/Debug.h>

#include <Content/Component/SCMapRenderer.h>
#include <Content/Script/CameraMoveScript.h>

#include <Content/SCData/Constants.h>
#include <Content/SCData/SCResourceManager.h>
#include <Content/SCData/SCEffectPass.h>

#include <Content/GameObject/SCSpriteObj.h>
#include <Content/GameObject/SCImageObj.h>
#include <Content/GameObject/SCUnitObj.h>
#include <Content/GameObject/SCSelectionCircleObj.h>
#include <Content/GameObject/SCHpBarObj.h>

#include <Content/Script/SCUnit.h>

#include <Content/HLSL/SCCommon.hlsli>

#include <SCSimulator/Include/SCSimulatorAPI.h>

// 적 시점 클로킹 확인 코드 ON/OFF 토글
#define ENEMY_CLOAK_MODE_CHECK 1

namespace // depth key
{
	// ---- 정렬 키 (31비트, 하위 → 상위) ----
	// [index 12][turret 1][y 13][elevation 5]
	// 상위 19비트 = OpenBW ui::sprite_depth_order (elevation | 지상이면 y, 공중이면 0 | turret)
	// 하위 index = 동점 처리. OpenBW는 sprite_t* 주소(힙 배치 의존)로 처리 → index 오름차순으로 결정적으로 대체
	constexpr uint32_t kIndexBits = 12;
	constexpr uint32_t kTurretBits = 1;
	constexpr uint32_t kYBits = 13;
	constexpr uint32_t kElevationBits = 5;
	constexpr uint32_t kIndexShift = 0;
	constexpr uint32_t kTurretShift = kIndexShift + kIndexBits;
	constexpr uint32_t kYShift = kTurretShift + kTurretBits;
	constexpr uint32_t kElevationShift = kYShift + kYBits;

	// ---- depth_key (17비트, 하위 → 상위) ----
	// [order 5][rank 12]
	// rank: 이번 싱크에서 보이는 스프라이트를 정렬 키로 정렬한 순위. 스프라이트마다 고유 → 키 동점 없음
	// order: 스프라이트 내 그리기 순번 (클수록 위). 보이는 이미지 + 선택 시 선택 원·체력 바 슬롯(최대 2개)을 포함
	constexpr uint32_t kOrderBits = 5;
	constexpr uint32_t kRankBits = 12;
	constexpr uint32_t kOrderShift = 0;
	constexpr uint32_t kRankShift = kOrderShift + kOrderBits;

	constexpr int32_t kGroundElevationMax = 4;	// 원작: elevation <= 4일 때만 y로 정렬

	static_assert(kElevationShift + kElevationBits <= 32, "정렬 키가 32비트를 넘음");
	static_assert(kRankShift + kRankBits <= 24, "+1 오프셋 포함 float(2^-24 정규화)로 정확히 표현 가능해야 함");
	static_assert(static_cast<uint32_t>(SCSim::kMaxSprites) <= (1u << kIndexBits));
	static_assert(static_cast<uint32_t>(SCSim::kMaxSprites) <= (1u << kRankBits));
	static_assert(static_cast<uint32_t>(SCSim::kMaxMapMegatiles) * 32u <= (1u << kYBits)); 

	uint32_t MakeSortKey(const SCSim::SpriteData& sprt_data)
	{
		ASSERT(sprt_data.elevation_level >= 0 && static_cast<uint32_t>(sprt_data.elevation_level) <= sc::Mask32(kElevationBits));
		ASSERT(sprt_data.index < static_cast<uint32_t>(SCSim::kMaxSprites));

		// Release에서 범위를 벗어나도 다른 필드를 침범하지 않도록 마스킹
		const uint32_t elevation = static_cast<uint32_t>(sprt_data.elevation_level) & sc::Mask32(kElevationBits);

		uint32_t y = 0;
		if (sprt_data.elevation_level <= kGroundElevationMax)
		{
			// 맵 높이 <= 8192는 LoadMap에서 보장
			y = static_cast<uint32_t>(std::max(sprt_data.y, 0)) & sc::Mask32(kYBits);
		}

		const uint32_t turret = (sprt_data.flags & SCSim::SpriteFlags::Turret) ? 1u : 0u;
		const uint32_t index = sprt_data.index & sc::Mask32(kIndexBits);

		return (elevation << kElevationShift) | (y << kYShift) | (turret << kTurretShift) | (index << kIndexShift);
	}

	uint32_t MakeDepthKey(uint32_t rank, uint32_t order)
	{
		ASSERT(rank <= sc::Mask32(kRankBits));
		ASSERT(order <= sc::Mask32(kOrderBits));

		return ((rank & sc::Mask32(kRankBits)) << kRankShift) | ((order & sc::Mask32(kOrderBits)) << kOrderShift);
	}
}

namespace sc
{
	namespace
	{
		// 원작 규칙: 로컬 플레이어 시점의 동맹 관계 (st.alliances[local][owner])
		// 중립 플레이어(11)·구출 플레이어는 OpenBW가 시작 시 Allied로 설정 → 별도 분기 불필요
		SelectionColorGroup GetSelectionColorGroup(int32_t owner, SCSim::Player local_player, const SCSim::PlayerData& local_player_data)
		{
			if (owner == static_cast<int32_t>(local_player))
			{
				return SelectionColorGroup::Own;
			}
			ASSERT(owner >= 0 && static_cast<size_t>(owner) < std::size(local_player_data.alliances));
			if (local_player_data.alliances[owner] != static_cast<int32_t>(SCSim::AllianceState::Enemy))
			{
				return SelectionColorGroup::NeutralOrAlly;
			}
			return SelectionColorGroup::Enemy;
		}
	}

	MainGameScene::MainGameScene()
		: Scene(MainGameScene::kClassConcreteName)
		, local_player_(SCSim::Player::_1)
	{
	}

	MainGameScene::~MainGameScene()
	{
		if (sc_sim_ctx_)
		{
			SCSim_Destroy(sc_sim_ctx_);
			sc_sim_ctx_ = nullptr;
		}
	}

	void MainGameScene::Init()
	{
		using namespace engine;

		Super::Init();

		DefaultEngineSetting();

		SCResourceManager::GetInst().Init();

		const auto& res_dir = ResourceManager::GetInst().GetResourceDir();
		const auto& mpq_dir = SCResourceManager::GetInst().GetMPQDir();

		sc_sim_ctx_ = SCSim_Create(mpq_dir.string().c_str());
		CHECK_F(sc_sim_ctx_ != nullptr, "Failed to create SCSimContext: {}", SCSim_GetLastError());

		std::filesystem::path map_path = res_dir / "SCMap" / "(4)Fighting Spirit 1.4.scx";
		bool result = SCSim_LoadMap(sc_sim_ctx_, map_path.string().c_str(), true);
		if (!result)
		{
			const char* err = SCSim_GetLastError();
			ENSURE_F(false, "Failed to load map: {}", err ? err : "Unknown error");

			return;
		}

		// SpriteTypeData(불변값이므로 1회만 시행)
		const SCSim::SpriteTypeData* types = nullptr;
		const int32_t type_count = SCSim_GetSpriteTypes(sc_sim_ctx_, &types);
		CHECK_F(type_count >= 0, "SCSim_GetSpriteTypes failed: {}", SCSim_GetLastError());
		sprite_types_ = std::span(types, static_cast<size_t>(type_count));

		{
			// Map Loading
			SCMapLoader& map_loader = SCResourceManager::GetInst().GetMapLoader();

			bool result = true;
			std::shared_ptr<Texture2D> map_tex;

			//CPU
			//auto sc_map = map_loader.LoadMapDataGPU(map_path);
			//ASSERT(result);

			//GPU
			std::unique_ptr<SCMap> sc_map = map_loader.LoadSCMap(map_path);

			//Save Map Texture to File
			//map_path = ResourceManager::GetInst().GetResourceDir();
			//map_path /= "MapTest.png";
			//map_tex->SaveToFile(map_path);

			GameObject* map_obj = AddGameObject<GameObject>();
			SCMapRenderer* map_rdr = map_obj->AddComponent<SCMapRenderer>();
			map_rdr->SetSCMap(std::move(sc_map));
			map_renderer_ = map_rdr;
		}

		GameObject* camobj = AddGameObject<GameObject>();
		camobj->SetName("MainCamera");
		camobj->GetTransform()->SetLocalPosition(float3(0.0f, 0.0f, -10.0f));
		auto cam = camobj->AddComponent<Camera>();
		Camera::ProjectionMatrixDesc proj_desc = cam->GetProjectionMatrixDesc();
		proj_desc.proj_mode = Camera::ProjectionMode::Orthographic;
		proj_desc.viewport_width = (float)sc::kResolutionWidth;
		proj_desc.viewport_height = (float)sc::kResolutionHeight;
		cam->CreateProjMatrix(proj_desc);
		CameraMoveScript* cam_script = camobj->AddComponent<CameraMoveScript>();
		engine::AABB2D map_bounds = {};
		map_bounds.min = float2(0.0f, 0.0f);
		auto* map_rdr = map_renderer_.get();
		CHECK(map_rdr);
		map_bounds.max = { 
			static_cast<float>(map_rdr->GetMapWidthMegatile() * static_cast<int32_t>(kMegatilePixelSize)), 
			static_cast<float>(map_rdr->GetMapHeightMegatile() * static_cast<int32_t>(kMegatilePixelSize)) 
		};
		CHECK(map_bounds.IsValid());
		cam_script->SetClampBounds(map_bounds);

		// ↑ 최소 기본값 세팅
		// ============================================================================================================
		// ↓ 테스트용 임시 코드

		// Ally 설정
		const SCSim::PlayerData* p_data_ptr = nullptr;
		int32_t player_num = SCSim_GetPlayers(sc_sim_ctx_, &p_data_ptr);
		ASSERT(0 <= player_num);
		memcpy(player_data_.data(), p_data_ptr, sizeof(SCSim::PlayerData) * static_cast<size_t>(player_num));
		
		player_data_[static_cast<size_t>(SCSim::Player::_3)].alliances[static_cast<size_t>(SCSim::Player::_1)] = static_cast<int32_t>(SCSim::AllianceState::Allied);
		player_data_[static_cast<size_t>(SCSim::Player::_1)].alliances[static_cast<size_t>(SCSim::Player::_3)] = static_cast<int32_t>(SCSim::AllianceState::Allied);

		result = SCSim_SetAlliances(sc_sim_ctx_, SCSim::Player::_3, player_data_[static_cast<size_t>(SCSim::Player::_3)].alliances, static_cast<int32_t>(player_data_.size()));
		ASSERT(result);
		result = SCSim_SetAlliances(sc_sim_ctx_, SCSim::Player::_1, player_data_[static_cast<size_t>(SCSim::Player::_1)].alliances, static_cast<int32_t>(player_data_.size()));
		ASSERT(result);


		uint32_t unit_id = SCSim_CreateUnit(sc_sim_ctx_, SCSim::UnitType::TerranMarine, SCSim::Player::_2, 2048, 2048);
		int32_t selected = SCSim_Select(sc_sim_ctx_, SCSim::Player::_1, &unit_id, 1);
		CHECK(selected != 0);
		{
			const SCSim::UnitData* units = nullptr;
			int32_t n = SCSim_GetUnits(sc_sim_ctx_, &units);
			// units[0..n)의 id, x, y, sprite_x, sprite_y 출력
		}

		{	// [테스트] action_select 다중 선택 검사 — 건물/유닛 순서
			const uint32_t marine_a = SCSim_CreateUnit(sc_sim_ctx_, SCSim::UnitType::TerranMarine, SCSim::Player::_1, 500, 500);
			const uint32_t marine_b = SCSim_CreateUnit(sc_sim_ctx_, SCSim::UnitType::TerranMarine, SCSim::Player::_1, 500, 500);

			const uint32_t barracks_a = SCSim_CreateUnit(sc_sim_ctx_, SCSim::UnitType::ProtossPylon, SCSim::Player::_1, 700, 700);
			const uint32_t barracks_b = SCSim_CreateUnit(sc_sim_ctx_, SCSim::UnitType::ProtossPylon, SCSim::Player::_1, 700, 400);

			LOG("ids: marine_a={}, marine_b={}, barracks_a={}, barracks_b={}", marine_a, marine_b, barracks_a, barracks_b);

			// 1. {유닛, 건물} → 예상 1
			const uint32_t case1[] = { marine_a, barracks_a };
			int32_t selected = SCSim_Select(sc_sim_ctx_, SCSim::Player::_1, case1, 2);
			LOG("case1 {{unit, building}}: {}", selected);

			// 2. {건물, 건물} → 예상 1
			const uint32_t case2[] = { barracks_a, barracks_b };
			selected = SCSim_Select(sc_sim_ctx_, SCSim::Player::_1, case2, 2);
			LOG("case2 {{building, building}}: {}", selected);

			// 3. {건물, 유닛, 유닛} → 예상 3 (혼합 선택)
			const uint32_t case3[] = { barracks_a, marine_a, marine_b };
			selected = SCSim_Select(sc_sim_ctx_, SCSim::Player::_1, case3, 3);
			LOG("case3 {{building, unit, unit}}: {}", selected);
		}

#define SPAWN_MARINE 0
#define SPAWN_WRAITH 0
#define SPAWN_DARK_TEMPLAR 1
#define SPAWN_ULTRALISK 1
		
#if SPAWN_MARINE
		{	// 마린
			constexpr int32_t kTestCount = 30;
			constexpr int32_t kBaseX = 2048;
			constexpr int32_t kBaseY = 2048;

			for (int32_t i = 0; i < kTestCount; ++i)
			{
				uint32_t test_id = SCSim::kInvalidUnitId;
					// 마린 충돌 폭(17px)보다 촘촘하게 → 스프라이트가 같은 y에서 겹치도록. 밀려나면 y가 달라질 수 있음
					constexpr int32_t kSpacingX = 8;
					test_id = SCSim_CreateUnit(sc_sim_ctx_, SCSim::UnitType::TerranMarine, SCSim::Player::_1, kBaseX + i * kSpacingX, kBaseY);
				CHECK_F(test_id != SCSim::kInvalidUnitId, "SCSim_CreateUnit failed: {}", SCSim_GetLastError());
			}
		}
#endif //SPAWN_MARINE


#if SPAWN_WRAITH
		{	// 테란 레이스
			constexpr int32_t kTestCount = 10;
			constexpr int32_t kBaseX = 2048;
			constexpr int32_t kBaseY = 2048;

			for (int32_t i = 0; i < kTestCount; ++i)
			{
				uint32_t test_id = SCSim::kInvalidUnitId;
				test_id = SCSim_CreateUnit(sc_sim_ctx_, SCSim::UnitType::TerranWraith, SCSim::Player::_2, kBaseX, kBaseY);
				CHECK_F(test_id != SCSim::kInvalidUnitId, "SCSim_CreateUnit failed: {}", SCSim_GetLastError());
			}
		}
#endif //SPAWN_WRAITH

#if SPAWN_DARK_TEMPLAR
		{
			constexpr int32_t kTestCount = 10;
			constexpr int32_t kBaseX = 2048;
			constexpr int32_t kBaseY = 2048;

			std::vector<uint32_t> dt_ids;
			dt_ids.reserve(kTestCount);
			for (int32_t i = 0; i < kTestCount; ++i)
			{
				uint32_t test_id = SCSim::kInvalidUnitId;
				test_id = SCSim_CreateUnit(sc_sim_ctx_, SCSim::UnitType::ProtossDarkTemplar, SCSim::Player::_3, kBaseX, kBaseY);
				CHECK_F(test_id != SCSim::kInvalidUnitId, "SCSim_CreateUnit failed: {}", SCSim_GetLastError());
				dt_ids.push_back(test_id);
			}

			// 이 유닛들을 선택
			SCSim_Select(sc_sim_ctx_, SCSim::Player::_1, dt_ids.data(), static_cast<int32_t>(dt_ids.size()));
		}
#endif //SPAWN_DARK_TEMPLAR

#if SPAWN_ULTRALISK
		{	// 울트라리스크
			constexpr int32_t kTestCount = 10;
			constexpr int32_t kBaseX = 2048;
			constexpr int32_t kBaseY = 2048;

			for (int32_t i = 0; i < kTestCount; ++i)
			{
				uint32_t test_id = SCSim::kInvalidUnitId;
				constexpr int32_t kSpacingX = 8;
				test_id = SCSim_CreateUnit(sc_sim_ctx_, SCSim::UnitType::ZergUltralisk, SCSim::Player::_1, kBaseX + i * kSpacingX, kBaseY);
				CHECK_F(test_id != SCSim::kInvalidUnitId, "SCSim_CreateUnit failed: {}", SCSim_GetLastError());
			}
		}
#endif //SPAWN_ULTRALISK


		{	// [테스트] EMP Shockwave (E 키)
			GameObject* emp_test_obj = AddGameObject<GameObject>();
			emp_test_obj->SetName("EmpTest");
			emp_test_obj->AddComponent("EmpTestScript");
		}

		{	// [테스트] 프로토스 건설 → WarpTexture / WarpFlash 확인 (B 키): 11시 방향
			GameObject* build_test_obj = AddGameObject<GameObject>();
			build_test_obj->SetName("BuildTest");
			build_test_obj->AddComponent("BuildTestScript");
		}

		{	// [테스트] 테란 커맨드센터 건설(V 키) → LiftOff / Land 확인 (L 키): 1시 방향
			GameObject* liftoff_test_obj = AddGameObject<GameObject>();
			liftoff_test_obj->SetName("LiftoffTest");
			liftoff_test_obj->AddComponent("LiftoffTestScript");
		}

		{	// [테스트] 저그 해처리 건설 (H키): 5시 방향
			GameObject* hatchery_test_obj = AddGameObject<GameObject>();
			hatchery_test_obj->SetName("HatcheryTest");
			hatchery_test_obj->AddComponent("HatcheryTestScript");
		}

		int32_t cur_frame = SCSim_GetCurrentFrame(sc_sim_ctx_);


	}
	void MainGameScene::FrameStart()
	{
		Super::FrameStart();

		ASSERT(sc_sim_ctx_ != nullptr);
		acc_delta_time_ += engine::TimeManager::GetInst().GetDeltaTime();
		while (acc_delta_time_ >= sc::kFrameTime)
		{
			acc_delta_time_ -= sc::kFrameTime;
			bool result = SCSim_NextFrame(sc_sim_ctx_);
			CHECK_F(result, "Failed to advance simulation to the next frame: {}", SCSim_GetLastError());
		}

		// Common Info Buffer Update
		SCCommonInfoCB info = {};
		info.view_origin = { 0, 0 };
		info.view_size = { sc::kResolutionWidth, sc::kResolutionHeight };
		sc_common_info_buffer_->Upload(engine::GraphicsDevice::GetInst().GetContext(), info);

		const SCSim::PlayerData* player_data = {};
		int32_t player_count = SCSim_GetPlayers(sc_sim_ctx_, &player_data);
		CHECK_F(player_count >= 0, "Failed to get players: {}", SCSim_GetLastError());
		CHECK(player_count <= player_data_.size());
		std::copy(player_data, player_data + player_count, player_data_.begin());

		// 유닛 Sync는 매 프레임 진행
		SyncUnits();
		SyncSprites();
		SyncMegatileStates();
	}
	void MainGameScene::PrepareMapLoader()
	{}
	void MainGameScene::DefaultEngineSetting()
	{
		using namespace engine;

		auto device = GraphicsDevice::GetInst().GetDevice();

		// Render Target creation
		RenderTargetPool& rt_pool = RenderManager::GetInst().GetRenderTargetPool();

		ColorTargetDesc color_desc = {};
		color_desc.create_srv = true;
		color_desc.create_uav = true;
		color_desc.format = DXGI_FORMAT_R32_UINT;
		color_desc.size = RenderTargetSize{ .base = RenderTargetSizeBase::Fixed, .width = (uint32_t)sc::kResolutionWidth, .height = (uint32_t)sc::kResolutionHeight, .scale = 1.0f };

		std::shared_ptr<RenderTarget> sc_rt = rt_pool.AcquireColor("ScWpeRenderTarget", color_desc);

		std::shared_ptr<RenderTargetGroup> sc_rendertarget_group = EntityManager::CreateEntity<RenderTargetGroup>();

		DepthTargetDesc depth_desc = {};
		depth_desc.format = DXGI_FORMAT_D32_FLOAT;
		depth_desc.size = RenderTargetSize{ .base = RenderTargetSizeBase::Fixed, .width = (uint32_t)sc::kResolutionWidth, .height = (uint32_t)sc::kResolutionHeight, .scale = 1.0f };
		std::shared_ptr<DepthStencilTarget> sc_dst = rt_pool.AcquireDepth("ScWpeDepthTarget", depth_desc);

		sc_rendertarget_group->SetRenderTargets({sc_rt, });
		sc_rendertarget_group->SetDepthStencilTarget(sc_dst);

		// Sc Opaque Pass
		std::unique_ptr<MeshRenderPass> sc_opaque = EntityManager::CreateEntity<MeshRenderPass>(kSCOpaquePassID, MeshRenderPass::SortPolicy::State);

		ClearSettings clear_setting = {};
		clear_setting.flags = ClearTarget::Color | ClearTarget::DepthStencil;
		clear_setting.depth = 0.0f; // 0으로 클리어(Greater equal 사용)
		clear_setting.color = { 0.0f, 0.0f, 0.0f, 0.0f };
		clear_setting.stencil = 0;

		sc_opaque->SetOutputTarget(sc_rendertarget_group, clear_setting);

		RenderManager::GetInst().RegisterRenderPass(std::move(sc_opaque));

		std::shared_ptr<RenderTarget> sc_wpe_mirror = rt_pool.AcquireColor("ScWpeMirror", color_desc);

		// SC Effect Pass
		std::unique_ptr<SCEffectPass> sc_effect_pass = EntityManager::CreateEntity<SCEffectPass>(kSCEffectPassID, MeshRenderPass::SortPolicy::Custom);
		sc_effect_pass->SetWPETargets(sc_rt, sc_dst, sc_wpe_mirror);
		RenderManager::GetInst().RegisterRenderPass(std::move(sc_effect_pass));

		// Sc Blit Pass
		{
			color_desc = {};
			color_desc.create_srv = true;
			color_desc.create_uav = false;
			color_desc.format = DXGI_FORMAT_R8G8B8A8_UNORM;
			color_desc.size = RenderTargetSize{ .base = RenderTargetSizeBase::Fixed, .width = (uint32_t)sc::kResolutionWidth, .height = (uint32_t)sc::kResolutionHeight, .scale = 1.0f };

			DepthTargetDesc depth_desc = {};
			depth_desc.format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			depth_desc.size = RenderTargetSize{ .base = RenderTargetSizeBase::Fixed, .width = (uint32_t)sc::kResolutionWidth, .height = (uint32_t)sc::kResolutionHeight, .scale = 1.0f };

			std::shared_ptr<RenderTargetGroup> sc_color_rtg = EntityManager::CreateEntity<RenderTargetGroup>();
			sc_color_rtg->SetRenderTargets({ rt_pool.AcquireColor("ScColorRenderTarget", color_desc), });
			sc_color_rtg->SetDepthStencilTarget(rt_pool.AcquireDepth("ScColorDepthTarget", depth_desc));

			clear_setting = {};
			clear_setting.flags = ClearTarget::Color | ClearTarget::DepthStencil;
			clear_setting.depth = 1.0f;
			clear_setting.color = { 0.0f, 0.0f, 0.0f, 1.0f };
			clear_setting.stencil = 0;

			std::unique_ptr<BlitPass> sc_blit = EntityManager::CreateEntity<BlitPass>(kScBlitPass);

			sc_blit->SetInputTexture(sc_rt);
			sc_blit->SetPixelShader(ResourceManager::GetInst().LoadFromFileWithoutAdd<PixelShader>("Shader/ScBlit_PS.cso"));

			//출력 RenderTarget은 지정하지 않음 -> Main RenderTarget에 출력
			RenderManager::GetInst().SetMainRenderTarget(sc_color_rtg);

			RenderManager::GetInst().RegisterRenderPass(std::move(sc_blit));
			RenderManager::GetInst().SetMainViewClearSettings(clear_setting);
		}

		std::vector<render_pass::ID> sc_pass_order = { kSCOpaquePassID, sc::kSCEffectPassID, kScBlitPass, render_pass::ID::Present };
#if ENABLE_DEBUG_DRAW
		sc_pass_order.push_back(render_pass::ID::DebugDraw);
#endif
		RenderManager::GetInst().SetRenderPassOrder(std::move(sc_pass_order));

		GetCollisionSystem2D()->SetCellSize(sc::kCellSize);
		SetCollisionMask(0, 0, true);


		// Common Info Buffer
		// 씬 종료 훅이 생기면 RemoveConstantBuffer 필요
		sc_common_info_buffer_ = EntityManager::CreateEntity<engine::ConstantBuffer>();
		ENSURE(sc_common_info_buffer_->Create<SCCommonInfoCB>());
		RenderManager::GetInst().GetPerFrameBindingTable().AddConstantBuffer(ShaderStage::Vertex | ShaderStage::Pixel, REG_B_SC_COMMON_INFO, sc_common_info_buffer_);
	}
	void MainGameScene::SyncUnits()
	{
		using namespace engine;

		const SCSim::UnitData* units = nullptr;
		int32_t count = SCSim_GetUnits(sc_sim_ctx_, &units);
		CHECK_F(count >= 0, "SCSim_GetUnits failed: {}", SCSim_GetLastError());

		++sync_serial_;
		
		for (int32_t i = 0; i < count; ++i) 
		{
			const SCSim::UnitData& snapshot = units[i];
			UnitEntry& entry = unit_objects_[snapshot.id];

			if (entry.object == nullptr) 
			{
				entry.object = AddGameObject<SCUnitObj>();
				entry.object->SetUnitID(snapshot.id);
			}

			// Serial Update
			entry.serial = sync_serial_;

			entry.object->GetSCUnit()->SetSnapshot(snapshot);

			const bool hidden = (snapshot.sprite_flags & SCSim::SpriteFlags::Hidden) != 0;

			entry.object->SetActive(!hidden);
		}

		// 이번에 못 본 유닛 = 죽었거나 사라짐
		for (auto it = unit_objects_.begin(); it != unit_objects_.end(); ) 
		{
			if (it->second.serial != sync_serial_) {
				if (SCUnitObj* obj = it->second.object) 
				{
					obj->Destroy();
				}
				it = unit_objects_.erase(it);
			}
			else 
			{
				++it;
			}
		}
		
	}

	void MainGameScene::SyncSprites()
	{
		SCSim::SpriteSnapshot snapshot = {};
		bool result = SCSim_GetSprites(sc_sim_ctx_, &snapshot);
		CHECK_F(result, "SCSim_GetSprites failed: {}", SCSim_GetLastError());

		const uint32_t* selection = nullptr;
		int32_t selected = SCSim_GetSelection(sc_sim_ctx_, local_player_, &selection);
		CHECK_F(0 <= selected, "SCSim_GetSelection failed: {}", SCSim_GetLastError());

		++sprite_sync_serial_;

		// 1) 현재 선택된 스프라이트 마킹해 두기
		for (int32_t i = 0; i < selected; ++i)
		{
			const uint32_t unit_id = selection[i];
			ASSERT(unit_id != SCSim::kInvalidUnitId);

			const auto iter = unit_objects_.find(unit_id);
			if(iter == unit_objects_.end()) 
			{
				DEBUG_LOG("Unit Sync 맞춰지지 않음: unit_id={}", unit_id);
				continue; 
			}

			const SCSim::UnitData& unit_data = iter->second.object->GetSCUnit()->GetSnapshot();

			ASSERT(unit_data.sprite_index < sprite_slots_.size());

			// Serial로 마킹, unit id도 기록(차후 Status Bar 렌더링에 필요한 Vital 데이터 찾는 데 필요)
			SpriteSlot& sprt_slot = sprite_slots_[unit_data.sprite_index];
			sprt_slot.selected_serial = sprite_sync_serial_;
			sprt_slot.selected_slot = static_cast<uint32_t>(i);	// 선택 순서

			// Unit ID만 기록, Serial은 아래에서 설정됨
			SelectionSlot& circle_slot = selection_pool_[i];
			circle_slot.unit_id = unit_id;
		}

		// 2) 보이는 스프라이트를 원작 그리기 순서로 정렬
		uint32_t visible_count = 0;
		for (int32_t i = 0; i < snapshot.sprite_count; ++i)
		{
			const SCSim::SpriteData& sprt_data = snapshot.sprites[i];

			// 숨김 스프라이트는 serial을 갱신하지 않음 → 아래 정리 루프에서 스프라이트/이미지 모두 비활성화
			if ((sprt_data.flags & SCSim::SpriteFlags::Hidden) != 0)
			{
				continue;
			}

			ASSERT(visible_count < sprite_sort_buffer_.size());

			// index를 정렬 조건에 추가해서 동점 제거
			sprite_sort_buffer_[visible_count] = (static_cast<uint64_t>(MakeSortKey(sprt_data)) << 32) | static_cast<uint32_t>(i);
			++visible_count;
		}
		std::sort(sprite_sort_buffer_.begin(), sprite_sort_buffer_.begin() + visible_count);

		// 2) 정렬 순서대로 처리. 루프 카운터가 곧 rank (먼저 = 아래)
		for (uint32_t rank = 0; rank < visible_count; ++rank)
		{
			const uint32_t snapshot_pos = static_cast<uint32_t>(sprite_sort_buffer_[rank] & 0xFFFFFFFFull);
			const SCSim::SpriteData& sprt_data = snapshot.sprites[snapshot_pos];

			ASSERT(sprt_data.index < sprite_slots_.size());
			ASSERT(sprt_data.owner >= 0 && static_cast<size_t>(sprt_data.owner) < player_data_.size());

			SpriteSlot& sprt_slot = sprite_slots_[sprt_data.index];	// 슬롯 번호로 조회 (sparse)

			const uint32_t player_color_idx = static_cast<uint32_t>(player_data_[sprt_data.owner].color);

			SCSpriteObj* sprt_obj = sprt_slot.object.get();
			if (sprt_obj == nullptr)
			{
				sprt_obj = AddGameObject<SCSpriteObj>();
				ASSERT(sprt_obj);
				sprt_slot.object = sprt_obj;
			}
			sprt_obj->ApplySnapshot(sprt_data);
			sprt_slot.serial = sprite_sync_serial_;
			sprt_obj->SetActive(true);

			ASSERT(sprt_data.first_image + sprt_data.image_count <= static_cast<uint32_t>(snapshot.image_count));

			// 첫 그림자 '직전'에 원을 끼워 넣어야 함 -> pending_circle = true로 마킹해 두고, 그림자가 아닌 이미지를 처음으로 만나면 끼워 넣고 off
			const bool is_selected = (sprt_slot.selected_serial == sprite_sync_serial_);
			bool circle_pending = is_selected;		// 아직 원을 끼우지 않음
			uint32_t circle_depth_key = 0;
			bool has_circle = false;

			uint32_t order = 0;
			// 아래 → 위 (OpenBW draw_sprite의 reverse(sprite->images)와 같은 방향)
			// 언더플로 방지를 위해 k - 1을 사용
			for (uint32_t k = sprt_data.image_count; k > 0; --k)
			{
				const uint32_t j = sprt_data.first_image + (k - 1);
				const SCSim::ImageData& img_data = snapshot.images[j];
				if ((img_data.flags & SCSim::ImageFlags::Hidden) != 0)
				{
					continue;
				}

				// 원작: 첫 비그림자 이미지 직전에 선택 원
				if (circle_pending && img_data.modifier != static_cast<int32_t>(SCSim::ImageModifier::Shadow))
				{
					// order ++로 중간에 끼운다
					circle_depth_key = MakeDepthKey(rank, order++);
					has_circle = true;
					circle_pending = false;
				}

				//이번 이미지 order
				const SCImageParam img_param = { .depth_key = MakeDepthKey(rank, order++), .player_color_idx = player_color_idx };

				ASSERT(img_data.index < image_slots_.size());
				ImageSlot& img_slot = image_slots_[img_data.index];	// 슬롯 번호로 조회 (sparse)

				SCImageObj* img_obj = img_slot.object.get();
				if (img_obj == nullptr)
				{
					img_obj = AddGameObject<SCImageObj>();
					ASSERT(img_obj);
					img_slot.object = img_obj;
					img_slot.sprite_index = SCSim::kInvalidSpriteIndex;	// 아래에서 반드시 SetParent
				}

				if (img_slot.sprite_index != img_data.sprite_index)
				{
					img_slot.sprite_index = img_data.sprite_index;
					img_obj->GetTransform()->SetParent(sprt_obj->GetTransform());
				}

				SCSim::ImageData resolved_img = img_data;
#if ENEMY_CLOAK_MODE_CHECK
				// [임시] 탐지 정보 연동 전까지 Player 3(테스트용 다크 템플러)의 Cloaked를 EnemyCloaked로 변환
				// 탐지 여부를 SCSim에서 받게 되면 '로컬 플레이어가 탐지하지 못한 적'으로 판정을 교체할 것
				
				if (sprt_data.owner == static_cast<int32_t>(SCSim::Player::_3))
				{
					switch (resolved_img.modifier)
					{
					case static_cast<int32_t>(SCSim::ImageModifier::Cloaking):
						resolved_img.modifier = static_cast<int32_t>(SCSim::ImageModifier::EnemyCloaking);
						break;
					case static_cast<int32_t>(SCSim::ImageModifier::Cloaked):
						resolved_img.modifier = static_cast<int32_t>(SCSim::ImageModifier::EnemyCloaked);
						break;
					case static_cast<int32_t>(SCSim::ImageModifier::Decloaking):
						resolved_img.modifier = static_cast<int32_t>(SCSim::ImageModifier::EnemyDecloaking);
						break;
					default:
						break;
					}
				}
#endif //ENEMY_CLOAK_MODE_CHECK

				// 매 프레임 바뀌기 때문에 ApplySnapshot을 항상 호출해줘야 함
				img_obj->ApplySnapshot(resolved_img, img_param);
				img_slot.serial = sprite_sync_serial_;
				img_obj->SetActive(true);
			}
			const uint32_t hp_bar_order = order++;	// 체력 바는 스프라이트 최전방에 그린다

			
			// 선택 원: is_selected && has_circle일 때만 (전부 그림자·숨김이면 원작도 원을 안 그림)
			// 체력 바: is_selected && 무적 아님 && Vital 데이터 있음
			if (is_selected)
			{
				ASSERT(sprt_slot.selected_slot < selection_pool_.size());
				SelectionSlot& sel = selection_pool_[sprt_slot.selected_slot];

				ASSERT(0 <= sprt_data.sprite_type && static_cast<size_t>(sprt_data.sprite_type) < sprite_types_.size());
				const SCSim::SpriteTypeData& sprt_type_data = sprite_types_[sprt_data.sprite_type];

				if (has_circle)
				{
					// 없을 시 생성
					SCSelectionCircleObj* sel_obj = sel.circle_obj.get();
					if (sel_obj == nullptr)
					{
						sel_obj = AddGameObject<SCSelectionCircleObj>();
						sel.circle_obj = sel_obj;
					}

					ASSERT(sel_obj);

					// 맨 마지막에 enum 상수를 넣어 그룹별 색상 확인 가능
					sel_obj->ApplySelection(sprt_data.x, sprt_data.y, sprt_type_data, circle_depth_key, GetSelectionColorGroup(sprt_data.owner, local_player_, player_data_[static_cast<size_t>(local_player_)]));

					sel_obj->SetActive(true);
					sel.circle_drawn_serial = sprite_sync_serial_;
				}

				const auto iter = unit_objects_.find(sel.unit_id);
				if (iter != unit_objects_.end())
				{

					ASSERT(iter->second.object);
					const SCSim::UnitData& unit_data = iter->second.object->GetSCUnit()->GetSnapshot();
					
					// 무적이 아닐 때만 HP Bar 렌더링
					if ((unit_data.status_flags & SCSim::UnitStatusFlags::Invincible) == 0)
					{
						SCSim::UnitVitals unit_vitals = {};
						bool get_vital_ok = SCSim_GetUnitVitals(sc_sim_ctx_, unit_data.id, &unit_vitals);

						if (get_vital_ok)
						{
							SCHpBarObj* hp_obj = sel.hp_bar_obj.get();
							if (hp_obj == nullptr)
							{
								hp_obj = AddGameObject<SCHpBarObj>();
								sel.hp_bar_obj = hp_obj;
							}
							ASSERT(hp_obj);

							hp_obj->ApplyHpBar(
								{
									.sprite_x = sprt_data.x,
									.sprite_y = sprt_data.y,
									.depth_key = MakeDepthKey(rank, hp_bar_order),
									.sprite_type_data = sprt_type_data,
									.unit_data = unit_data,
									.vitals = unit_vitals,
								}
								);

							hp_obj->SetActive(true);
							sel.hp_bar_drawn_serial = sprite_sync_serial_;
						}
						else
						{
							DEBUG_LOG("Failed to get unit vitals for unit_id={}: {}", unit_data.id, SCSim_GetLastError());
						}
					}
				}
			}
		}

		// 이번 싱크에서 못 본 슬롯 비활성화 (슬롯 배열 전체)
		for (SpriteSlot& slot : sprite_slots_)
		{
			if (slot.serial != sprite_sync_serial_)
			{
				if (SCSpriteObj* obj = slot.object.get())
				{
					obj->SetActive(false);
				}
			}
		}

		for (ImageSlot& slot : image_slots_)
		{
			if (slot.serial != sprite_sync_serial_)
			{
				if (SCImageObj* obj = slot.object.get())
				{
					obj->SetActive(false);
				}
			}
		}

		for (SelectionSlot& slot : selection_pool_)
		{
			if (slot.circle_drawn_serial != sprite_sync_serial_)
			{
				if (SCSelectionCircleObj* obj = slot.circle_obj.get())
				{
					obj->SetActive(false);
				}
			}

			if (slot.hp_bar_drawn_serial != sprite_sync_serial_)
			{
				if (SCHpBarObj* obj = slot.hp_bar_obj.get())
				{
					obj->SetActive(false);
				}
			}
		}
	}
	void MainGameScene::SyncMegatileStates()
	{
		SCMapRenderer* renderer = map_renderer_.get();
		if (renderer == nullptr)
		{
			return;
		}

		const SCSim::MegatileState* megatiles = nullptr;
		uint32_t map_width_megatile = 0;
		uint32_t map_height_megatile = 0;
		const bool got = SCSim_GetMegatileStates(sc_sim_ctx_, &megatiles, &map_width_megatile, &map_height_megatile);
		CHECK_F(got, "SCSim_GetMegatileStates failed: {}", SCSim_GetLastError());

		const bool uploaded = renderer->ApplyMegatileSnapshot(megatiles, map_width_megatile, map_height_megatile);
		CHECK_F(uploaded, "Failed to upload tile snapshot");
	}
}

