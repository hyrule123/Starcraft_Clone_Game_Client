#include "Content/pch.h"
#include "TestGameObject.h"

#include <Engine/Game/Component/Transform.h>
#include <Engine/Game/Component/Renderer.h>

#include <Engine/Resource/Mesh/Mesh.h>
#include <Engine/Resource/Material/Material.h>

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
		s_ptr<Mesh> mesh = resmgr.Find<Mesh>("Debug_Mesh");
		s_ptr<Material> material = resmgr.Find<Material>("Debug_Material");

		ASSERT(mesh);
		ASSERT(material);

		renderer->SetMesh(mesh);
		renderer->SetMaterial(material);
	}
}