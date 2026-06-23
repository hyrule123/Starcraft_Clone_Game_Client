#include "Content/pch.h"
#include "TestGameObject.h"

#include <Content/ControlScript.h>

#include <Engine/Game/Component/Transform.h>
#include <Engine/Game/Component/Renderer.h>

#include <Engine/Resource/Mesh/Mesh.h>
#include <Engine/Resource/Material/Material.h>
#include <Engine/Resource/Texture2D.h>

#include <Engine/Manager/ResourceManager.h>

#include <Engine/Core/Debug.h>

#include <Engine/HLSL/CppShared/Register.hlsli>

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

		auto renderer = AddComponent<Renderer>();

		auto& resmgr = ResourceManager::GetInst();
		s_ptr<Mesh> mesh = resmgr.Find<Mesh>("SpriteMesh");
		s_ptr<Material> material = resmgr.Find<Material>("SpriteMaterial");
		
		s_ptr<Texture2D> tex = resmgr.LoadFromFile<Texture2D>("Texture2D/Magenta.png");
		material->SetTexture(tex, SLOT_T_BASE_COLOR);

		ASSERT(mesh);
		ASSERT(material);

		renderer->SetMesh(mesh);
		renderer->SetMaterial(material);

		AddComponent<ControlScript>();
	}
}