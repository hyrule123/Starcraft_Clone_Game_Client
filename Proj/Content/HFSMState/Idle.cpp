#include "Content/pch.h"
#include "Idle.h"

#include <Engine/Core/Debug.h>
#include <Engine/Core/Math.h>

#include <Engine/Manager/TimeManager.h>

#include <Engine/Game/Component/HFSM.h>
#include <Engine/Game/Component/BlackBoard.h>



#include <array>

#include <Engine/Game/Component/SpriteAnimator.h>

namespace engine
{
	constexpr std::array <HashedStringView, 9> kIdleAnimNames = {
		"Idle_0"_hash,
		"Idle_1"_hash,
		"Idle_2"_hash,
		"Idle_3"_hash,
		"Idle_4"_hash,
		"Idle_5"_hash,
		"Idle_6"_hash,
		"Idle_7"_hash,
		"Idle_8"_hash,
	};

	constexpr std::array <HashedStringView, 9> kMoveAnimNames = {
	"Move_0"_hash,
	"Move_1"_hash,
	"Move_2"_hash,
	"Move_3"_hash,
	"Move_4"_hash,
	"Move_5"_hash,
	"Move_6"_hash,
	"Move_7"_hash,
	"Move_8"_hash,
	};

	Idle::Idle()
		: Super(Idle::kClassConcreteName)
	{}
	Idle::~Idle()
	{}
	void Idle::OnAwake(const AIContext& ai_context)
	{
		Super::OnAwake(ai_context);
		
		ASSERT(ai_context.black_board);

		playing_animation_ = ai_context.black_board->GetValue<HashedStringView>("PlayingAnimation"_hash);
		ASSERT(playing_animation_);
	}
	void Idle::OnEnter(const AIContext& ai_context)
	{
		Super::OnEnter(ai_context);
		//Idle에 진입했을 때 수행할 로직 작성
		DEBUG_LOG("Idle::OnEnter() called.");

		*playing_animation_ = "Idle"_hash;
	}
	void Idle::OnExit(const AIContext& ai_context)
	{
		Super::OnExit(ai_context);

		DEBUG_LOG("Idle::OnExit() called.");
	}
	void Idle::OnUpdate(const AIContext& ai_context)
	{
		Super::OnUpdate(ai_context);

	}
	HashedStringView Idle::CheckTransition(const AIContext& ai_context)
	{
		return ""_hash;
	}
}