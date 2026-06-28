#include "Content/pch.h"
#include "TerranMarine.h"

#include <Engine/Manager/ResourceManager.h>

#include <Engine/Resource/Graphics/Buffer/Texture2DArray.h>
#include <Engine/Resource/Graphics/Material.h>
#include <Engine/Resource/SpriteAnimation.h>

#include <Engine/Game/Component/Transform.h>
#include <Engine/Game/Component/SpriteRenderer.h>
#include <Engine/Game/Component/SpriteAnimator.h>

#include <Engine/Core/Debug.h>


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

		auto renderer = AddComponent<SpriteRenderer>();
		auto animator = AddComponent<SpriteAnimator>();

		auto& res_mgr = ResourceManager::GetInst();

		anim_ = res_mgr.Find<SpriteAnimation>("Marine_SpriteAnimation");
		if (!anim_)
		{
			s_ptr<Texture2DArray> marine_sprite =
				res_mgr.LoadFromFileWithoutAdd<Texture2DArray>("Texture2D/SC/Terran/Marine.bmp");
			ASSERT((bool)marine_sprite);

			bool result = marine_sprite->Slice(14, 17);
			ASSERT(result);

			anim_ = std::make_shared<SpriteAnimation>();
			res_mgr.AddResource("Marine_SpriteAnimation", anim_);
			anim_->SetSprite(marine_sprite);

			AnimationClip clip = {};
			for (uint32 i = 0; i < 9; ++i)
			{
				uint32 frame = 68u + 17u * i;
				clip.frames.push_back(frame);
			}
			clip.duration = 2.0f;
			clip.is_loop = true;

			anim_->AddAnimationClip("Walk_Forward", clip);
		}
		animator->SetSpriteAnimation(anim_);
		animator->Play("Walk_Forward");
	}
}

