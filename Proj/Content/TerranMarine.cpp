#include "Content/pch.h"
#include "TerranMarine.h"

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
	constexpr uint32 n_row = 14u, n_col = 17u;
	constexpr float tick_time = 23.81f;

	TerranMarine::TerranMarine()
		:Super(STRINGIFY(TerranMarine))
	{}

	TerranMarine::~TerranMarine()
	{}
	void TerranMarine::Init()
	{
		Super::Init();

		auto tr = GetTransform();
		tr->SetLocalScale({ 300.0f, 300.0f, 1.0f });
		tr->SetLocalPosition({ 10.0f, 10.0f, 10.0f });

		auto renderer = AddComponent<SpriteRenderer>();
		auto animator = AddComponent<SpriteAnimator>();

		auto& res_mgr = ResourceManager::GetInst();

		anim_ = res_mgr.Find<SpriteAnimation>("Marine_SpriteAnimation");
		if (!anim_)
		{
			s_ptr<Texture2DArray> marine_sprite =
				res_mgr.LoadFromFileWithoutAdd<Texture2DArray>("Texture2D/SC/Terran/marine.png");
			ASSERT((bool)marine_sprite);

			bool result = marine_sprite->Slice(n_row, n_col);
			ASSERT(result);

			anim_ = std::make_shared<SpriteAnimation>();
			res_mgr.AddResource("Marine_SpriteAnimation", anim_);
			anim_->SetSprite(marine_sprite);
			
			std::vector<uint32> frames = {};
			std::vector<SpriteAnimClip::Frame> frame_with_time = {};
			//총 18개 direction
			for (uint32 i = 0; i < 9; ++i)
			{
				//Idle
				u_ptr<SpriteAnimClip> idle_clip = std::make_unique<SpriteAnimClip>();
				frames.clear();
				frames.push_back(i * 2);
				idle_clip->AddFrames(frames, 1.0f);
				idle_clip->SetLoop(false);
				anim_->AddAnimationClip("Idle_" + std::to_string(i), std::move(idle_clip));

				//Prepare Attack
				u_ptr<SpriteAnimClip> attack_windup = std::make_unique<SpriteAnimClip>();
				frames.clear();
				for (uint32 j = 0; j < 3; ++j)
				{
					uint32 frame = i + n_col * j;
					frames.push_back(frame);
				}
				attack_windup->AddFrames(frames, 0.2f);
				attack_windup->SetLoop(false);
				anim_->AddAnimationClip("AttackWindup_" + std::to_string(i), std::move(attack_windup));

				//Attack
				u_ptr<SpriteAnimClip> attack_clip = std::make_unique<SpriteAnimClip>();
				frame_with_time.clear();
				float attack_total_duration = 15.0f / tick_time;
				float flash_time = 0.07f;
				frame_with_time.push_back({ i + n_col * 3, flash_time });
				frame_with_time.push_back({ i + n_col * 2, flash_time });
				frame_with_time.push_back({ i + n_col * 3, flash_time });
				frame_with_time.push_back({ i + n_col * 2, attack_total_duration - flash_time * 3.0f });
				attack_clip->AddFrames(std::move(frame_with_time));
				attack_clip->SetLoop(true);
				anim_->AddAnimationClip("Attack_" + std::to_string(i), std::move(attack_clip));

				//Move
				u_ptr<SpriteAnimClip> move_clip = std::make_unique<SpriteAnimClip>();

				uint32 start_frame = 68u + (i * 2);
				frames.clear();
				for (uint32 j = 0; j < 9; ++j)
				{
					uint32 frame = start_frame + n_col * j;
					frames.push_back(frame);
				}

				move_clip->AddFrames(frames, 0.4f);
				move_clip->SetLoop(true);
				anim_->AddAnimationClip("Move_" + std::to_string(i), std::move(move_clip));
			}

			//Death
			u_ptr<SpriteAnimClip> death_clip = std::make_unique<SpriteAnimClip>();
			frames.clear();
			uint32 frame_start = n_col * (n_row - 1);
			for (uint32 i = 0; i < 8; ++i)
			{
				frames.push_back(frame_start + i);
			}
			death_clip->AddFrames(frames, 0.8f);
			death_clip->SetLoop(false);
			anim_->AddAnimationClip("Death", std::move(death_clip));

			//Rot - 이 애니메이션은 GameObject를 새로 생성해야할 듯함(다른 이미지에 있음)
		}
		animator->SetSpriteAnimation(anim_);
		animator->Play("Move_8");
	}
}

