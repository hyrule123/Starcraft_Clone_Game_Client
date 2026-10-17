#include "Content/pch.h"
#include "MainGameScene.h"

#include <Engine/Manager/GraphicsDevice.h>
#include <Engine/Manager/ResourceManager.h>
#include <Engine/Manager/RenderManager.h>
#include <Engine/Manager/TimeManager.h>

#include <Engine/Resource/GPU/RenderTargetGroup.h>
#include <Engine/Resource/GPU/Buffer/RenderTarget.h>
#include <Engine/Resource/GPU/Buffer/DepthStencilTarget.h>
#include <Engine/Resource/GPU/Buffer/TypedBuffer.h>
#include <Engine/Resource/GPU/Material.h>
#include <Engine/Resource/GPU/Mesh.h>
#include <Engine/Resource/GPU/PipelineState.h>
#include <Engine/Resource/GPU/Shader/PixelShader.h>

#include <Engine/Render/RenderPass/MeshRenderPass.h>
#include <Engine/Render/RenderPass/BlitPass.h>

#include <Engine/Game/GameObject.h>
#include <Engine/Game/Component/Camera.h>
#include <Engine/Game/Component/SpriteRenderer.h>
#include <Engine/Game/Component/Transform.h>

#include <Content/Legacy/GameObject/TerranMarine.h>
#include <Content/Legacy/GameObject/TerranMedic.h>
#include <Content/GameObject/SCUnitObj.h>
#include <Content/GameObject/SCUnitDebug.h>
#include <Content/Script/SCUnit.h>

#include <Content/Legacy/Script/ActiveEnableTest.h>
#include <Content/Legacy/Script/TestScript.h>

#include <Content/Script/DestroyTestScript.h>
#include <Content/Script/SCUnit.h>

#include <Content/Component/SCMapRenderer.h>

#include <Content/HLSL/SCMapRender.hlsli>

#include <Content/SCData/Constants.h>
#include <Content/SCData/SCImageLibrary.h>
#include <Content/SCData/MPQ/MPQFileSystem.h>
#include <Content/SCData/MPQ/MPQArchive.h>
#include <Content/SCData/SCTileSetPcxLUT.h>
#include <Content/SCData/SCResourceManager.h>
#include <Content/SCData/SCMapBakeComputePass.h>

#include <Content/GameObject/SCSpriteObj.h>
#include <Content/GameObject/SCImageObj.h>

#include <Engine/Core/Debug.h>
#include <SCSimulator/Include/SCSimulatorAPI.h>

namespace // depth key
{
	// ---- depth_key 비트 배치 (하위 → 상위) ----
	// [order 4][turret 1][mid 13][elevation 5] = 23비트
	// mid: 지상(elevation <= 4)은 position.y, 공중은 sprite index(원작의 포인터 주소 동점 처리 대체)

	constexpr const uint32_t kOrderBits = 4;
	constexpr const uint32_t kTurretBits = 1;
	constexpr const uint32_t kMidBits = 13;
	constexpr const uint32_t kElevationBits = 5;
	constexpr const uint32_t kOrderShift = 0;
	constexpr const uint32_t kTurretShift = kOrderShift + kOrderBits;
	constexpr const uint32_t kMidShift = kTurretShift + kTurretBits;
	constexpr const uint32_t kElevationShift = kMidShift + kMidBits;

	constexpr int kGroundElevationMax = 4;	// 원작: elevation <= 4일 때만 y로 정렬

	static_assert(SCSim::kMaxSprites - 1 <= sc::Mask32(kMidBits));

	static_assert(kOrderBits + kTurretBits + kMidBits + kElevationBits == 23, "Depth key changed.");

	uint32_t MakeDepthBase(const SCSim::SpriteData& sprt_data)
	{
		ASSERT(sprt_data.elevation_level >= 0 && static_cast<uint32_t>(sprt_data.elevation_level) <= sc::Mask32(kElevationBits));

		// Release에서 범위를 벗어나도 키가 23비트를 넘지 않도록 마스킹
		const uint32_t elevation = static_cast<uint32_t>(sprt_data.elevation_level) & sc::Mask32(kElevationBits);

		uint32_t mid = 0;
		if (sprt_data.elevation_level <= kGroundElevationMax)
		{
			// 맵 높이 <= 8192는 LoadMap에서 보장
			// 지상 unit은 y sorting
			mid = static_cast<uint32_t>(std::max(sprt_data.y, 0)) & sc::Mask32(kMidBits);
		}
		else
		{
			// 공중 유닛은 index 기준으로 sorting(원작에서는 memory 주소로 소팅하여 fighting 방지)
			mid = static_cast<uint32_t>(sprt_data.index) & sc::Mask32(kMidBits);
		}

		const uint32_t turret = (sprt_data.flags & SCSim::SpriteFlags::Turret) ? 1u : 0u;

		return (elevation << kElevationShift) | (mid << kMidShift) | (turret << kTurretShift);
	}

	uint32_t MakeDepthKey(uint32_t depth_base, uint32_t order)
	{
		ASSERT(order <= sc::Mask32(kOrderBits));

		constexpr uint32_t kOrderMask = sc::Mask32(kOrderBits) << kOrderShift;
		ASSERT_F((depth_base & kOrderMask) == 0, "Order 자리는 비워져있어야 합니다.");

		return depth_base | ((order & sc::Mask32(kOrderBits)) << kOrderShift);
	}
}

namespace sc
{
	MainGameScene::MainGameScene()
		: Scene(MainGameScene::kClassConcreteName)
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

		{
			// Map Loading
			SCResourceManager& map_loader = SCResourceManager::GetInst();

			bool result = true;
			std::shared_ptr<Texture2D> map_tex;

			//CPU
			//auto sc_map = map_loader.LoadMapDataGPU(map_path);
			//ASSERT(result);

			//GPU
			auto sc_map = map_loader.LoadMapDataGPU(map_path);

			//Save Map Texture to File
			//map_path = ResourceManager::GetInst().GetResourceDir();
			//map_path /= "MapTest.png";
			//map_tex->SaveToFile(map_path);

			GameObject* map_obj = AddGameObject<GameObject>();
			SCMapRenderer* renderer = map_obj->AddComponent<SCMapRenderer>();

			renderer->SetSCMap(std::move(sc_map));
		}

		uint32_t unit_id = SCSim_CreateUnit(sc_sim_ctx_, SCSim::UnitType::TerranMarine, SCSim::Player::_2, 2048, 2048);
		int32_t selected = SCSim_Select(sc_sim_ctx_, SCSim::Player::_1, &unit_id, 1);
		CHECK(selected != 0);
		{
			const SCSim::UnitData* units = nullptr;
			int32_t n = SCSim_GetUnits(sc_sim_ctx_, &units);
			// units[0..n)의 id, x, y, sprite_x, sprite_y 출력
		}

		// ---- 깊이 키 테스트 ----
		{
			constexpr bool kTestAir = false;		// true: 공중 100기 같은 위치 / false: 지상 가로 한 줄(같은 y)
			constexpr int32_t kTestCount = 10;
			constexpr int32_t kBaseX = 2048;
			constexpr int32_t kBaseY = 2048;

			for (int32_t i = 0; i < kTestCount; ++i)
			{
				uint32_t test_id = SCSim::kInvalidUnitId;
				if (kTestAir)
				{
					test_id = SCSim_CreateUnit(sc_sim_ctx_, SCSim::UnitType::TerranWraith, SCSim::Player::_1, kBaseX, kBaseY);
				}
				else
				{
					// 마린 충돌 폭(17px)보다 촘촘하게 → 스프라이트가 같은 y에서 겹치도록. 밀려나면 y가 달라질 수 있음
					constexpr int32_t kSpacingX = 8;
					test_id = SCSim_CreateUnit(sc_sim_ctx_, SCSim::UnitType::TerranMarine, SCSim::Player::_1, kBaseX + i * kSpacingX, kBaseY);
				}
				CHECK_F(test_id != SCSim::kInvalidUnitId, "SCSim_CreateUnit failed: {}", SCSim_GetLastError());
			}
		}

		//SCUnitDebug* u = AddGameObject<SCUnitDebug>();
		//CHECK(u);

		//u->SetUnitId(unit_id);


		//SCSim_Order(sc_sim_ctx_, SCSim::Player::_1, SCSim::OrderType::Move, 2048, 2048, SCSim::kInvalidUnitId, -1, false);

		//for (int i = 0; i < 2000; ++i)
		//{
		//	SCSim_NextFrame(sc_sim_ctx_);

		//	if (i % 24 == 0) {
		//		
		//		SCSim::UnitData unit;
		//		bool found = SCSim_GetUnit(sc_sim_ctx_, unit_id, &unit);
		//		CHECK(found);
		//		LOG("Frame {}: Unit ID {} position ({}, {})", SCSim_GetCurrentFrame(sc_sim_ctx_), unit_id, unit.x, unit.y);

		//		//const SCSimUnit* units = nullptr;
		//		//int32_t n = SCSim_GetUnits(sc_sim_ctx_, &units);
		//		//for (int32_t i = 0; i < n; ++i)
		//		//{
		//		//	if (units[i].id == unit_id)
		//		//	{
		//		//		LOG("Frame {}: Unit ID {} position ({}, {})", SCSim_GetCurrentFrame(sc_sim_ctx_), unit_id, units[i].x, units[i].y);
		//		//		break;
		//		//	}
		//		//}
		//	}
		//}

		int32_t cur_frame = SCSim_GetCurrentFrame(sc_sim_ctx_);

		GameObject* camobj = AddGameObject<GameObject>();
		camobj->SetName("MainCamera");
		camobj->GetTransform()->SetLocalPosition(float3(0.0f, 0.0f, -10.0f));
		auto cam = camobj->AddComponent<Camera>();
		Camera::ProjectionMatrixDesc proj_desc = cam->GetProjectionMatrixDesc();
		proj_desc.proj_mode = Camera::ProjectionMode::Orthographic;
		proj_desc.viewport_width = (float)sc::kResolutionWidth;
		proj_desc.viewport_height = (float)sc::kResolutionHeight;
		cam->CreateProjMatrix(proj_desc);
		camobj->AddComponent("CameraMoveScript");

		//auto marine = AddGameObject("TerranMarine");
		
		//for (size_t i = 0; i < 30; ++i)
		//{
		//	AddGameObject("TestGameObject");
		//}



		//auto test_script = camobj->AddComponent<TestScript>();
		//test_script->SetTargetEnableObject(marine);


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

		const SCSim::PlayerData* player_data = {};
		int32_t player_count = SCSim_GetPlayers(sc_sim_ctx_, &player_data);
		CHECK_F(player_count >= 0, "Failed to get players: {}", SCSim_GetLastError());
		CHECK(player_count <= player_data_.size());
		std::copy(player_data, player_data + player_count, player_data_.begin());
		

		// 유닛 Sync는 매 프레임 진행
		//SyncUnits();
		SyncSprites();
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
		color_desc.size = RenderTargetSize{ .base = RenderTargetSizeBase::Fixed, .width = (uint32)sc::kResolutionWidth, .height = (uint32)sc::kResolutionHeight, .scale = 1.0f };

		std::shared_ptr<RenderTarget> sc_rt = rt_pool.AcquireColor("ScWpeRenderTarget", color_desc);

		std::shared_ptr<RenderTargetGroup> sc_rendertarget_group = EntityManager::CreateEntity<RenderTargetGroup>();

		DepthTargetDesc depth_desc = {};
		depth_desc.format = DXGI_FORMAT_D32_FLOAT;
		depth_desc.size = RenderTargetSize{ .base = RenderTargetSizeBase::Fixed, .width = (uint32)sc::kResolutionWidth, .height = (uint32)sc::kResolutionHeight, .scale = 1.0f };
		std::shared_ptr<DepthStencilTarget> sc_dst = rt_pool.AcquireDepth("ScWpeDepthTarget", depth_desc);

		sc_rendertarget_group->SetRenderTargets({sc_rt, });
		sc_rendertarget_group->SetDepthStencilTarget(sc_dst);

		// Sc Opaque Pass
		std::unique_ptr<MeshRenderPass> sc_opaque = EntityManager::CreateEntity<MeshRenderPass>(kScOpaquePass, MeshRenderPass::SortPolicy::State);

		ClearSettings clear_setting = {};
		clear_setting.flags = ClearTarget::Color | ClearTarget::DepthStencil;
		clear_setting.depth = 0.0f; // 0으로 클리어(Greater equal 사용)
		clear_setting.color = { 0.0f, 0.0f, 0.0f, 0.0f };
		clear_setting.stencil = 0;

		sc_opaque->SetOutputTarget(sc_rendertarget_group, clear_setting);

		RenderManager::GetInst().RegisterRenderPass(std::move(sc_opaque));

		// Sc Blit Pass
		{
			color_desc = {};
			color_desc.create_srv = true;
			color_desc.create_uav = false;
			color_desc.format = DXGI_FORMAT_R8G8B8A8_UNORM;
			color_desc.size = RenderTargetSize{ .base = RenderTargetSizeBase::Fixed, .width = (uint32)sc::kResolutionWidth, .height = (uint32)sc::kResolutionHeight, .scale = 1.0f };

			DepthTargetDesc depth_desc = {};
			depth_desc.format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			depth_desc.size = RenderTargetSize{ .base = RenderTargetSizeBase::Fixed, .width = (uint32)sc::kResolutionWidth, .height = (uint32)sc::kResolutionHeight, .scale = 1.0f };

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

		std::vector<render_pass::ID> sc_pass_order = { kScOpaquePass, kScBlitPass, render_pass::ID::Present };
#if ENABLE_DEBUG_DRAW
		sc_pass_order.push_back(render_pass::ID::DebugDraw);
#endif
		RenderManager::GetInst().SetRenderPassOrder(std::move(sc_pass_order));


		GetCollisionSystem2D()->SetCellSize(sc::kCellSize);
		SetCollisionMask(0, 0, true);

		// 맵 데이터 이미지로 저장하는 코드
		/*
		bool load_result = map_loader.LoadMapDataCPU(map_path);

		std::shared_ptr<Texture2D> map_tex = map_loader.GetMapTexture();

		map_tex->SaveToFile(ResourceManager::GetInst().GetResourceDir() / L"MapTexture.png");

		GameObject* map_obj = AddGameObject<GameObject>();
		map_obj->SetName("MapObject");
		SpriteRenderer* renderer = map_obj->AddComponent<SpriteRenderer>();
		std::shared_ptr<Material> map_mtrl = renderer->GetMaterial()->Clone();
		map_mtrl->SetTexture(map_tex, 0);
		renderer->SetMaterial(map_mtrl);

		map_obj->GetTransform()->SetLocalPosition({ 0.0f, 0.0f, 100.0f });
		map_obj->GetTransform()->SetLocalScale({(float)map_tex->GetWidth(), (float)map_tex->GetHeight(), 1.0f});
		*/
	}
	void MainGameScene::SyncUnits()
	{
		CHECK_F(false, "Needs refactoring");

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
				// 새 유닛인 경우
				entry.object = AddGameObject<SCUnitObj>();
				entry.object->SetUnitID(snapshot.id);
			}

			// Serial Update
			entry.serial = sync_serial_;

			entry.object->GetComponent<SCUnit>()->SetSnapshot(snapshot);

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

		++sprite_sync_serial_;

		for (int32_t i = 0; i < snapshot.sprite_count; ++i)
		{
			const SCSim::SpriteData& sprt_data = snapshot.sprites[i];

			// 숨김 스프라이트는 serial을 갱신하지 않음 → 아래 정리 루프에서 스프라이트/이미지 모두 비활성화
			if ((sprt_data.flags & SCSim::SpriteFlags::Hidden) != 0)
			{
				continue;
			}

			ASSERT(sprt_data.index < sprite_slots_.size());
			ASSERT(sprt_data.owner >= 0 && static_cast<size_t>(sprt_data.owner) < player_data_.size());

			SpriteSlot& sprt_slot = sprite_slots_[sprt_data.index];	// 슬롯 번호로 조회 (sparse)

			const uint32_t player_color_idx = static_cast<uint32_t>(player_data_[sprt_data.owner].color);
			const uint32_t depth_base = MakeDepthBase(sprt_data);

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

			const uint32_t img_end = sprt_data.first_image + sprt_data.image_count;
			ASSERT(img_end <= static_cast<uint32_t>(snapshot.image_count));

			for (uint32_t j = sprt_data.first_image; j < img_end; ++j)
			{
				const SCSim::ImageData& img_data = snapshot.images[j];

				// 숨김 이미지도 배열 위치는 차지함. order를 j로 계산하므로 건너뛰어도 다른 이미지의 order는 유지됨
				if ((img_data.flags & SCSim::ImageFlags::Hidden) != 0)
				{
					continue;
				}

				ASSERT(img_data.index < image_slots_.size());

				// 배열은 앞 = 맨 위층. order는 클수록 위에 그려지므로 뒤집음
				const uint32_t order = (sprt_data.image_count - 1) - (j - sprt_data.first_image);

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

				const SCImageParam img_param = { .depth_key = MakeDepthKey(depth_base, order), .player_color = player_color_idx };

				// 매 프레임 바뀌기 때문에 ApplySnapshot을 항상 호출해줘야 함
				img_obj->ApplySnapshot(img_data, img_param);
				img_slot.serial = sprite_sync_serial_;
				img_obj->SetActive(true);
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
	}
}

