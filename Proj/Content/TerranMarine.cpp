#include "Content/pch.h"
#include "TerranMarine.h"

#include <Engine/Manager/ResourceManager.h>

#include <Engine/Resource/Graphics/Buffer/SpriteTextureArray.h>
#include <Engine/Resource/Graphics/Material.h>

#include <Engine/Game/Component/Transform.h>
#include <Engine/Game/Component/SpriteRenderer.h>

namespace engine
{
	TerranMarine::TerranMarine()
		:Super(STRINGIFY(TerranMarine))
	{}

	TerranMarine::~TerranMarine()
	{}
	void TerranMarine::Init()
	{
		Super::Init();

		auto tr = GetTransform();
		tr->SetLocalScale({ 1000.f, 1000.f, 1.0f });
		tr->SetLocalPosition({ 10.0f, 10.0f, 10.0f });

		auto& res_mgr = ResourceManager::GetInst();

		mtrl_ = res_mgr.Find<Material>("Marine_Material");
		if (!mtrl_)
		{
			mtrl_ = ResourceManager::GetInst().Find<Material>("Sprite_Material")->Clone();
			ResourceManager::GetInst().AddResource("Marine_Material", mtrl_);

			s_ptr<SpriteTextureArray> marine_tex = ResourceManager::GetInst().LoadFromFile<SpriteTextureArray>("Texture2D/SC/Terran/Marine.bmp");
			marine_tex->CreateSpriteFromAtlas(14, 17);

			mtrl_->SetTextures({ marine_tex, });
		}
		auto sprdr = AddComponent<SpriteRenderer>();
		sprdr->SetMaterial(mtrl_);
	}
}

