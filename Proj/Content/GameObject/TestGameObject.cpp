#include "Content/pch.h"
#include "TestGameObject.h"

#include <Content/Legacy/Script/ControlScript.h>

#include <Engine/Game/Component/Transform.h>
#include <Engine/Game/Component/SpriteRenderer.h>
#include <Engine/Game/Component/CircleCollider2D.h>
#include <Engine/Game/Component/AABBCollider2D.h>

#include <Engine/Resource/GPU/Mesh.h>
#include <Engine/Resource/GPU/Material.h>
#include <Engine/Resource/GPU/Buffer/Texture2D.h>

#include <Engine/Manager/ResourceManager.h>

#include <Engine/Core/Debug.h>

#include <Engine/HLSL/Core/Register.hlsli>

#include <random>

namespace sc
{
	TestGameObject::TestGameObject()
	{
	}
	TestGameObject::~TestGameObject()
	{
	}
	void TestGameObject::Init()
	{
		using namespace engine;

		Super::Init();

		static std::random_device rand_dev;
		static std::mt19937 rand_gen(rand_dev());
		static std::uniform_real_distribution<float> float_dist_x(1800.0f, 2200.0f);
		static std::uniform_real_distribution<float> float_dist_y(-2200.0f, -1800.0f);

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
		//std::shared_ptr<Mesh> mesh = resmgr.Find<Mesh>("Sprite_Mesh");
		//std::shared_ptr<Material> material = resmgr.Find<Material>("Sprite_Material");
		//
		//std::shared_ptr<Texture2D> tex = resmgr.LoadFromFile<Texture2D>("Texture2D/Magenta.png");
		//material->SetTexture(tex, REG_T_BASE_COLOR);

		//ASSERT(mesh);
		//ASSERT(material);

		//renderer->SetMesh(mesh);
		//renderer->SetMaterial(material);

		//AddComponent<ControlScript>();
	}
}