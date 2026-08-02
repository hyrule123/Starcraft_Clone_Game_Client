#include "Content/pch.h"
#include "TestGameObject.h"

#include <Content/Script/ControlScript.h>

#include <Engine/Game/Component/Transform.h>
#include <Engine/Game/Component/SpriteRenderer.h>
#include <Engine/Game/Component/CircleCollider2D.h>
#include <Engine/Game/Component/AABBCollider2D.h>

#include <Engine/Resource/Graphics/Mesh.h>
#include <Engine/Resource/Graphics/Material.h>
#include <Engine/Resource/Graphics/Buffer/Texture2D.h>

#include <Engine/Manager/ResourceManager.h>

#include <Engine/Core/Debug.h>

#include <Engine/HLSL/CppShared/Register.hlsli>

#include <random>

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

		static std::random_device rand_dev;
		static std::mt19937 rand_gen(rand_dev());
		static std::uniform_real_distribution<float> float_dist_x(-320.0f, 320.0f);
		static std::uniform_real_distribution<float> float_dist_y(-240.0f, 240.0f);

		float2 rand_pos(float_dist_x(rand_gen), float_dist_y(rand_gen));
		GetTransform()->SetLocalPosition(float3(rand_pos.x, rand_pos.y, 1.0f));
		
		static bool toggle = false;
		toggle = !toggle;
		if (toggle)
		{
			auto circle_collider = AddComponent<CircleCollider2D>();
			static std::uniform_real_distribution<float> radius_dist(16.0f, 64.0f);
			circle_collider->SetRadius(radius_dist(rand_gen));
		}
		else
		{
			auto aabb_collider = AddComponent<AABBCollider2D>();
			static std::uniform_real_distribution<float> size_dist(16.0f, 64.0f);
			aabb_collider->SetSize(float2(size_dist(rand_gen), size_dist(rand_gen)));
		}

		//auto renderer = AddComponent<SpriteRenderer>();

		//auto& resmgr = ResourceManager::GetInst();
		//s_ptr<Mesh> mesh = resmgr.Find<Mesh>("Sprite_Mesh");
		//s_ptr<Material> material = resmgr.Find<Material>("Sprite_Material");
		//
		//s_ptr<Texture2D> tex = resmgr.LoadFromFile<Texture2D>("Texture2D/Magenta.png");
		//material->SetTexture(tex, SLOT_T_BASE_COLOR);

		//ASSERT(mesh);
		//ASSERT(material);

		//renderer->SetMesh(mesh);
		//renderer->SetMaterial(material);

		//AddComponent<ControlScript>();
	}
}