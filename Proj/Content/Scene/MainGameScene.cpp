#include "Content/pch.h"
#include "MainGameScene.h"

#include <Engine/Manager/GraphicsDevice.h>
#include <Engine/Manager/ResourceManager.h>
#include <Engine/Manager/RenderManager.h>


#include <Engine/Resource/Graphics/RenderTargetGroup.h>
#include <Engine/Resource/Graphics/Buffer/RenderTargetView.h>
#include <Engine/Resource/Graphics/Buffer/DepthStencilView.h>
#include <Engine/Resource/Graphics/Material.h>
#include <Engine/Resource/Graphics/Mesh.h>
#include <Engine/Resource/Graphics/GraphicsShaderSet.h>

#include <Engine/Game/GameObject.h>
#include <Engine/Game/Component/Camera.h>
#include <Engine/Game/Component/Transform.h>

#include <Engine/Core/Debug.h>

#include <Content/GameObject/TerranMarine.h>
#include <Content/GameObject/TerranMedic.h>

#include <Content/SCData/Constant.h>

#include <Content/Script/DestroyTestScript.h>
#include <Content/Script/ActiveEnableTest.h>

namespace engine
{
	MainGameScene::MainGameScene()
		: Scene(MainGameScene::kClassConcreteName)
	{
	}

	MainGameScene::~MainGameScene()
	{
	}

	void MainGameScene::Init()
	{
		Super::Init();

		LoadResources();

		auto one = AddGameObject("TerranMarine"_hash);
		one->AddComponent<ActiveEnableTest>();

		s_ptr<GameObject> camobj = AddGameObject<GameObject>();
		camobj->SetName("MainCamera");
		auto cam = camobj->AddComponent<Camera>();
		Camera::ProjectionMatrixDesc proj_desc = cam->GetProjectionMatrixDesc();
		proj_desc.proj_mode = Camera::ProjectionMode::Orthographic;
		proj_desc.viewport_width = (float)SC::kResolutionWidth;
		proj_desc.viewport_height = (float)SC::kResolutionHeight;
		cam->CreateProjMatrix(proj_desc);
	}
	void MainGameScene::LoadResources()
	{
		auto& res_mgr = ResourceManager::GetInst();
		auto& resmgr = ResourceManager::GetInst();

		auto device = GraphicsDevice::GetInst().GetDevice();

		// Render Target
		s_ptr<RenderTargetGroup> sc_rendertarget_group = std::make_shared<RenderTargetGroup>();
		s_ptr<RenderTargetView> sc_rtv = std::make_shared<RenderTargetView>();

		
		bool result = sc_rtv->CreateDefault((UINT)SC::kResolutionWidth, (UINT)SC::kResolutionHeight, DXGI_FORMAT_R8G8B8A8_UNORM);
		ASSERT_RELEASE(result);

		//최종 렌더타겟에 이어붙일 때는 SRV가 필요
		sc_rtv->CreateSRV(nullptr);

		s_ptr<DepthStencilView> dsv = std::make_shared<DepthStencilView>();
		D3D11_TEXTURE2D_DESC depth_buffer_desc = {};
		depth_buffer_desc.Width = (UINT)SC::kResolutionWidth;            // RTV의 가로 크기와 완전히 일치해야 합니다.
		depth_buffer_desc.Height = (UINT)SC::kResolutionHeight;          // RTV의 세로 크기와 완전히 일치해야 합니다.
		depth_buffer_desc.MipLevels = 1;               // 깊이 버퍼는 밉맵이 필요 없으므로 1 고정입니다.
		depth_buffer_desc.ArraySize = 1;

		// Reversed-Z 사용 시 f32 포맷 사용
		// 차후 Stencil 버퍼 필요 시 버퍼 공간을 확장
		depth_buffer_desc.Format = DXGI_FORMAT_D32_FLOAT;
		// 멀티샘플링 설정 (RTV 생성할 때 넣은 Count, Quality 값과 무조건 일치해야 에러가 안 납니다)
		depth_buffer_desc.SampleDesc.Count = 1;
		depth_buffer_desc.SampleDesc.Quality = 0;

		depth_buffer_desc.Usage = D3D11_USAGE_DEFAULT;          // GPU가 매 프레임 읽고 쓸 것이므로 DEFAULT
		depth_buffer_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL; // 이 텍스처의 용도는 깊이/스텐실 도화지임을 명시
		depth_buffer_desc.CPUAccessFlags = 0;
		depth_buffer_desc.MiscFlags = 0;
		result = dsv->CreateTexture2D(&depth_buffer_desc);
		ASSERT_RELEASE(result);

		sc_rendertarget_group->SetRenderTargets({sc_rtv, });
		sc_rendertarget_group->SetDepthStencilView(dsv);

		//렌더링
		RenderManager::GetInst().GetOpaquePass()->SetRenderTargetGroup(sc_rendertarget_group);
		RenderManager::GetInst().GetPresentPass()->SetSourceRenderTarget(sc_rendertarget_group);
	}
}

