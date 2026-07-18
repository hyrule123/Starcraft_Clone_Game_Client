#include "Content/pch.h"
#include "UnitInputHandler.h"

#include <Engine/Manager/InputManager.h>
#include <Engine/Manager/RenderManager.h>
#include <Engine/Core/EngineMain.h>

#include <Engine/Game/Component/Camera.h>
#include <Engine/Game/Component/Transform.h>

#include <Engine/Game/Component/BlackBoard.h>


#include <Engine/Core/Debug.h>

#include <Content/SCData/Constant.h>

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

		s_ptr<BlackBoard> bb = GetOwner()->GetComponent<BlackBoard>();
		ASSERT(bb);
		blackboard_ = bb;

		command_input_ = bb->GetValue<HashedStringView>("CommandInput"_hash);
		destination_ = bb->GetValue<float2>("Destination"_hash);

		ASSERT(command_input_);
		ASSERT(destination_);
	}
	void UnitInputHandler::Update()
	{
		Super::Update();

		//이동명령
		if (input_mgr_.GetKeyDown(KeyCode::kMouseRight))
		{
			//스크린 좌표계 기준 마우스 위치
			float2 mouse_pos = { (float)input_mgr_.GetMousePos().x, (float)input_mgr_.GetMousePos().y };

			auto cam = RenderManager::GetInst().GetMainCamera();
			ASSERT(cam);

			//게임 해상도와 현재 윈도우 해상도가 다를 수 있으므로 비율을 구해서 곱해주어야 함
			constexpr float width_ratio = (float)SC::kResolutionWidth / (float)kDefaultWidth;
			constexpr float height_ratio = (float)SC::kResolutionHeight / (float)kDefaultHeight;

			static_assert(kDefaultWidth == 1280U && kDefaultHeight == 960U, "Engine Default Resolution has changed, please update the code accordingly.");

			mouse_pos.x *= width_ratio;
			mouse_pos.y *= height_ratio;

			//1. 화면의 가운데가 원점 -> 일치시키기
			//윈도우 기준 좌상단이 0, 0 / 게임 기준 좌상단 -width / 2, height / 2
			//윈도우 기준 우하단이 width, height / 게임 기준 width / 2, height / 2
			// 선형 변환 시 GameX = (WindowX - width / 2), GameY = (height / 2 - WindowY)
			mouse_pos.x = (mouse_pos.x - SC::kResolutionWidth * 0.5f);
			mouse_pos.y = (SC::kResolutionHeight * 0.5f - mouse_pos.y);

			//여기에 카메라의 위치를 더해주면 월드 좌표계(Transform이 완성되지 않았으므로 일단은 LocalPosition으로) 기준 좌표로 변환됨
			const auto& cam_pos = cam->GetOwner()->GetTransform()->GetLocalPosition();
			mouse_pos.x += cam_pos.x;
			mouse_pos.y += cam_pos.y;
			
			//우클릭 강제이동
			*command_input_ = "ForceMove"_hash;
			*destination_ = mouse_pos;
		}
	}
}