#include "Content/pch.h"
#include "SCUnitDebug.h"

#include <Engine/Manager/ResourceManager.h>

#include <Engine/Resource/GPU/Material.h>
#include <Engine/Resource/GPU/PipelineState.h>

#include <Engine/Game/Component/Transform.h>

namespace sc
{
	SCUnitDebug::SCUnitDebug()
		: GameObject(SCUnitDebug::kClassConcreteName)
	{}
	SCUnitDebug::~SCUnitDebug()
	{}
	void SCUnitDebug::Init()
	{
		Super::Init();

		auto* p = AddComponent("SCSpriteDebugRenderer");
		CHECK(p != nullptr);
		
		using namespace engine;

		Transform* transform = GetComponent<Transform>();
		transform->SetLocalScale(float3(100.0f, 100.0f, 1.0f));
		transform->SetLocalPosition(float3(0.f, 0.f, 10.0f));
	}
}