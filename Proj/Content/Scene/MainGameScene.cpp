#include "Content/pch.h"
#include "MainGameScene.h"

#include <Engine/Manager/GraphicsDevice.h>
#include <Engine/Manager/ResourceManager.h>
#include <Engine/Manager/RenderManager.h>
#include <Engine/Manager/TimeManager.h>

#include <Engine/Resource/GPU/RenderTargetGroup.h>
#include <Engine/Resource/GPU/Buffer/RenderTargetView.h>
#include <Engine/Resource/GPU/Buffer/DepthStencilView.h>
#include <Engine/Resource/GPU/Buffer/TypedBuffer.h>


#include <Engine/Resource/GPU/Material.h>
#include <Engine/Resource/GPU/Mesh.h>
#include <Engine/Resource/GPU/PipelineState.h>

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

#include <Content/SCData/SCResourceManager.h>
#include <Content/SCData/SCMapBakeComputePass.h>

#include <Content/Component/SCMapRenderer.h>

#include <Content/HLSL/SCMapRender.hlsli>

#include <Content/SCData/Constants.h>
#include <Content/SCData/SCImageLibrary.h>
#include <Content/SCData/MPQ/MPQFileSystem.h>
#include <Content/SCData/MPQ/MPQArchive.h>
#include <Content/SCData/SCTileSetPcxLUT.h>
#include <Content/GameObject/SCSpriteObj.h>
#include <Content/GameObject/SCImageObj.h>

#include <Engine/Core/Debug.h>
#include <SCSimulator/Include/SCSimulatorAPI.h>

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

		uint32_t unit_id = SCSim_CreateUnit(sc_sim_ctx_, SCSim::UnitType::TerranMarine, SCSim::Player::_1, 2048, 2048);
		int32_t selected = SCSim_Select(sc_sim_ctx_, SCSim::Player::_1, &unit_id, 1);
		CHECK(selected != 0);
		{
			const SCSim::UnitData* units = nullptr;
			int32_t n = SCSim_GetUnits(sc_sim_ctx_, &units);
			// units[0..n)의 id, x, y, sprite_x, sprite_y 출력
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
		
		for (size_t i = 0; i < 30; ++i)
		{
			AddGameObject("TestGameObject");
		}



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

		// Render Target
		std::shared_ptr<RenderTargetGroup> sc_rendertarget_group = EntityManager::CreateEntity<RenderTargetGroup>();
		std::shared_ptr<RenderTargetView> sc_rtv = EntityManager::CreateEntity<RenderTargetView>();
		
		bool result = sc_rtv->CreateDefaultBuffer((UINT)sc::kResolutionWidth, (UINT)sc::kResolutionHeight, DXGI_FORMAT_R8G8B8A8_UNORM);
		CHECK(result);

		//최종 렌더타겟에 이어붙일 때는 SRV가 필요
		result = sc_rtv->CreateSRV(nullptr);
		CHECK(result);

		std::shared_ptr<DepthStencilView> dsv = EntityManager::CreateEntity<DepthStencilView>();
		D3D11_TEXTURE2D_DESC depth_buffer_desc = {};
		depth_buffer_desc.Width = (UINT)sc::kResolutionWidth;            // RTV의 가로 크기와 완전히 일치해야 합니다.
		depth_buffer_desc.Height = (UINT)sc::kResolutionHeight;          // RTV의 세로 크기와 완전히 일치해야 합니다.
		depth_buffer_desc.MipLevels = 1;               // 깊이 버퍼는 밉맵이 필요 없으므로 1 고정입니다.
		depth_buffer_desc.ArraySize = 1;

		// Reversed-Z 사용 시 f32 포맷 사용
		// 차후 Stencil 버퍼 필요 시 버퍼 공간을 확장
		depth_buffer_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		// 멀티샘플링 설정 (RTV 생성할 때 넣은 Count, Quality 값과 무조건 일치해야 에러가 안 납니다)
		depth_buffer_desc.SampleDesc.Count = 1;
		depth_buffer_desc.SampleDesc.Quality = 0;

		depth_buffer_desc.Usage = D3D11_USAGE_DEFAULT;          // GPU가 매 프레임 읽고 쓸 것이므로 DEFAULT
		depth_buffer_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL; // 이 텍스처의 용도는 깊이/스텐실 도화지임을 명시
		depth_buffer_desc.CPUAccessFlags = 0;
		depth_buffer_desc.MiscFlags = 0;
		result = dsv->CreateTexture2D(&depth_buffer_desc);
		CHECK(result);
		result = dsv->CreateDSV(nullptr);
		CHECK(result);

		sc_rendertarget_group->SetRenderTargets({sc_rtv, });
		sc_rendertarget_group->SetDepthStencilView(dsv);

		//렌더링
		RenderManager::GetInst().SetMainRenderTarget(sc_rendertarget_group);

		GetCollisionSystem2D()->SetCellSize(sc::kCellSize);
		SetCollisionMask(0, 0, true);
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

			const bool hidden = (snapshot.flags & SCSim::UnitFlags::Hidden) != 0;
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
			SpriteSlot& sprt_slot = sprite_slots_[sprt_data.index];	// 슬롯 번호로 조회

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

			const int32_t img_end = sprt_data.first_image + sprt_data.image_count;
			ASSERT(img_end <= snapshot.image_count);

			for (int32_t j = sprt_data.first_image; j < img_end; ++j)
			{
				const SCSim::ImageData& img_data = snapshot.images[j];
				ImageSlot& img_slot = image_slots_[img_data.index];	// 슬롯 번호로 조회

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

				// 매 프레임 바뀌기 때문에 ApplySnapshot을 항상 호출해줘야 함
				img_obj->ApplySnapshot(img_data, sprt_data.player_color);
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

