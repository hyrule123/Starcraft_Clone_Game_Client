#include "Content/pch.h"
#include "TestGameObject.h"

#include <Engine/Game/Component/Transform.h>
#include <Engine/Game/Component/Renderer.h>

#include <Engine/Resource/Mesh/Mesh.h>

#include <Engine/Manager/ResourceManager.h>

#include <Engine/Core/Debug.h>

namespace engine
{
	TestGameObject::TestGameObject()
	{
	}
	TestGameObject::~TestGameObject()
	{
	}
	void TestGameObject::Init()
	{
		Super::Init();

		AddComponent<Transform>();
		auto renderer = AddComponent<Renderer>();

		auto& resmgr = ResourceManager::GetInst();
		s_ptr<Mesh> debugmesh = resmgr.Find<Mesh>("DebugMesh");

		ASSERT(debugmesh);

		renderer->SetMesh(debugmesh);
	}
}