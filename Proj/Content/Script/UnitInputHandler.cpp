#include "Content/pch.h"
#include "UnitInputHandler.h"

#include <Engine/Manager/InputManager.h>
#include <Engine/Manager/RenderManager.h>
#include <Engine/Manager/GraphicsDevice.h>

#include <Engine/Game/Component/Camera.h>
#include <Engine/Game/Component/Transform.h>
#include <Engine/Game/Component/Blackboard.h>

#include <Engine/Core/Debug.h>

namespace engine
{
	UnitInputHandler::UnitInputHandler()
		: Super(UnitInputHandler::kClassConcreteName)
		, input_mgr_(InputManager::GetInst())
	{}
	UnitInputHandler::~UnitInputHandler()
	{}
	void UnitInputHandler::Awake()
	{
		Super::Awake();

		blackboard_ = GetOwner()->GetComponent<Blackboard>();
		ASSERT(!blackboard_.expired());
	}
	void UnitInputHandler::Update()
	{
		Super::Update();

		//이동명령
		if (input_mgr_.GetKeyDown(KeyCode::kMouseRight))
		{
			float2 mouse_pos = { (float)input_mgr_.GetMousePos().x, (float)input_mgr_.GetMousePos().y };

			auto cam = RenderManager::GetInst().GetMainCamera();
			ASSERT(cam);

			//1. 화면의 가운데가 원점 -> 일치시키기

			//일단은 스왑체인의 해상도를 사용
			float2 screen_center = { GraphicsDevice::GetInst().GetResolutionWidth() / 2.0f, GraphicsDevice::GetInst().GetResolutionHeight() / 2.0f };

			mouse_pos -= screen_center;

			//2. Y축 반전
			mouse_pos.y = -mouse_pos.y;

			//일단은 수동 계산
			const auto& view_mat = cam->GetViewMatrix();
			mouse_pos.x = mouse_pos.x - view_mat._41;
			mouse_pos.y = mouse_pos.y - view_mat._42;

			int a = 3;

			blackboard_.lock()->SetValue("Destination"_hash, mouse_pos);
		}
	}
}