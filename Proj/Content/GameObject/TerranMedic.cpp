#include "Content/pch.h"
#include "TerranMedic.h"

#include <Engine/Manager/ResourceManager.h>

#include <Engine/Resource/Graphics/Buffer/Texture2DArray.h>

#include <Engine/Resource/SpriteAnimClip.h>
#include <Engine/Resource/SpriteAnimation.h>

#include <Engine/Game/Component/Transform.h>
#include <Engine/Game/Component/SpriteRenderer.h>
#include <Engine/Game/Component/SpriteAnimator.h>

#include <Engine/Core/Debug.h>


namespace engine
{
	TerranMedic::TerranMedic()
		:Super(TerranMedic::kClassConcreteName)
	{}

	TerranMedic::~TerranMedic()
	{}
	void TerranMedic::Init()
	{
		Super::Init();

		auto tr = GetTransform();
		tr->SetLocalScale({ 300.0f, 300.0f, 1.0f });
		tr->SetLocalPosition({ 10.0f, 10.0f, 10.0f });

		//auto renderer = AddComponent<SpriteRenderer>();
		//auto animator = AddComponent<SpriteAnimator>();

		//auto& res_mgr = ResourceManager::GetInst();

		//anim_ = res_mgr.Find<SpriteAnimation>("Medic_SpriteAnimation");
		//if (!anim_)
		//{
		//	s_ptr<Texture2DArray> medic_sprite =
		//		res_mgr.LoadFromFileWithoutAdd<Texture2DArray>("Texture2D/SC/Terran/medic.bmp");
		//	ASSERT((bool)medic_sprite);

		//	bool result = medic_sprite->Slice(14, 17);
		//	ASSERT(result);

		//	anim_ = EntityManager::CreateEntity<SpriteAnimation>();
		//	res_mgr.AddResource("Medic_SpriteAnimation", anim_);
		//	anim_->SetSprite(medic_sprite);

		//	u_ptr<SpriteAnimClip> clip = std::make_unique<SpriteAnimClip>();
		//	std::vector<uint32> frames = {};
		//	for (uint32 i = 0; i < 9; ++i)
		//	{
		//		uint32 frame = 68u + 17u * i;
		//		frames.push_back(frame);
		//	}
		//	clip->AddFrames(frames, 0.75f);
		//	clip->SetLoop(true);

		//	anim_->AddAnimationClip("Walk_Forward", std::move(clip));
		//}
		//animator->SetSpriteAnimation(anim_);
		//animator->Play("Walk_Forward");
	}
}

