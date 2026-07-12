#include "Content/pch.h"
#include "TerranMarine.h"

#include <Engine/Manager/ResourceManager.h>

#include <Engine/Resource/Graphics/Buffer/Texture2DArray.h>

#include <Engine/Resource/SpriteAnimClip.h>
#include <Engine/Resource/SpriteAnimation.h>
#include <Engine/Resource/Graphics/Material.h>

#include <Engine/Game/Component/Transform.h>
#include <Engine/Game/Component/SpriteRenderer.h>
#include <Engine/Game/Component/SpriteAnimator.h>
#include <Engine/Game/Component/HFSM.h>
#include <Engine/Game/Component/Blackboard.h>

#include <Content/Script/IdleState.h>
#include <Content/Script/MoveState.h>
#include <Content/Script/UnitInputHandler.h>

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
		auto hfsm = AddComponent<HFSM>();
		auto blackboard = AddComponent<Blackboard>();
		auto unit_input_handler = AddComponent<UnitInputHandler>();
		

		auto& res_mgr = ResourceManager::GetInst();

		anim_ = res_mgr.Find<SpriteAnimation>("Marine_SpriteAnimation"_hash);
		if (!anim_)
		{
			s_ptr<Texture2DArray> marine_sprite =
				res_mgr.LoadFromFileWithoutAdd<Texture2DArray>("Texture2D/SC/Terran/marine.png"_hash);
			ASSERT((bool)marine_sprite);

			bool result = marine_sprite->Slice(n_row, n_col);
			ASSERT(result);

			anim_ = std::make_shared<SpriteAnimation>();
			res_mgr.AddResource("Marine_SpriteAnimation"_hash, anim_);
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

				HashedString idle_clip_name = "Idle_" + std::to_string(i);

				anim_->AddAnimationClip(idle_clip_name, std::move(idle_clip));

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

				HashedString attack_windup_name = "AttackWindup_" + std::to_string(i);
				anim_->AddAnimationClip(attack_windup_name, std::move(attack_windup));

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

				HashedString attack_clip_name = "Attack_" + std::to_string(i);
				anim_->AddAnimationClip(attack_clip_name, std::move(attack_clip));

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
				HashedString move_clip_name = "Move_" + std::to_string(i);
				anim_->AddAnimationClip(move_clip_name, std::move(move_clip));
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
			anim_->AddAnimationClip("Death"_hash, std::move(death_clip));

			//Rot - 이 애니메이션은 GameObject를 새로 생성해야할 듯함(다른 이미지에 있음)
		}
		animator->SetSpriteAnimation(anim_);
		animator->Play("Move_8"_hash);

		//해당 Animation만의 Material Key 생성.
		constexpr HashedStringView mtrl_key = "Material_Marine"_hash;

		//애니메이션 재생에 사용할 텍스처가 등록된 고유 Material을 찾는다
		s_ptr<Material> mtrl = res_mgr.Find<Material>(mtrl_key);

		//고유 Material이 없을 경우 새로 생성 후 Renderer에 텍스처 지정
		if (!mtrl)
		{
			mtrl = res_mgr.Find<Material>("Material_Sprite"_hash);
			ASSERT((bool)mtrl);
			mtrl = mtrl->Clone();
			mtrl->SetTextures({ anim_->GetSprite(), });

			res_mgr.AddResource(mtrl_key, mtrl);
		}
		renderer->SetMaterial(mtrl);

		auto idle_state = std::make_unique<IdleState>();
		idle_state->SetParentState(hfsm->GetRootState());
		
		auto move_state = std::make_unique<MoveState>();
		move_state->SetParentState(hfsm->GetRootState());

		hfsm->AddState("Idle"_hash, std::move(idle_state));
		hfsm->AddState("Move"_hash, std::move(move_state));
		hfsm->SetInitialState("Idle"_hash);
	}
}

