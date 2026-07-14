#include "Content/pch.h"
#include "UnitInputHandler.h"

#include <Engine/Manager/InputManager.h>
#include <Engine/Manager/RenderManager.h>
#include <Engine/Manager/GraphicsDevice.h>

#include <Engine/Game/Component/Camera.h>
#include <Engine/Game/Component/Transform.h>

#include <Engine/Game/Component/Blackboard.h>

#include <Engine/Core/Debug.h>

#include <Content/SCConstants/Command.h>

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
			//윈도우 기준 좌상단이 0, 0 / 게임 기준 좌상단 -width / 2, height / 2
			//윈도우 기준 우하단이 width, height / 게임 기준 width / 2, height / 2
			// 선형 변환 시 GameX = (WindowX - width / 2), GameY = (height / 2 - WindowY)
			//일단은 스왑체인의 해상도를 사용
			mouse_pos.x = mouse_pos.x - (float)(GraphicsDevice::GetInst().GetResolutionWidth()) * 0.5f;
			mouse_pos.y = (float)(GraphicsDevice::GetInst().GetResolutionHeight()) * 0.5f - mouse_pos.y;

			//여기에 카메라의 위치를 더해주면 월드 좌표계(Transform이 완성되지 않았으므로 일단은 LocalPosition으로) 기준 좌표로 변환됨
			const auto& cam_pos = cam->GetOwner()->GetTransform()->GetLocalPosition();
			mouse_pos.x += cam_pos.x;
			mouse_pos.y += cam_pos.y;

			s_ptr<Blackboard> bb = blackboard_.lock();
			
			bb->SetValue("Command"_hash, Command(MoveCommand(mouse_pos)));
		}
	}
}