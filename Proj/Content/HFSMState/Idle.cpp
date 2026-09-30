#include "Content/pch.h"
#include "Idle.h"

#include <Engine/Core/Debug.h>
#include <Engine/Core/Math.h>

#include <Engine/Manager/TimeManager.h>

#include <Engine/Game/Component/HFSM.h>
#include <Engine/Game/Component/BlackBoard.h>



#include <array>

#include <Engine/Game/Component/SpriteAnimator.h>

namespace sc
{
	using namespace engine::literals;
	constexpr std::array <engine::HashedStringView, 9> kIdleAnimNames = {
	"Idle_0",
	"Idle_1",
	"Idle_2",
	"Idle_3",
	"Idle_4",
	"Idle_5",
	"Idle_6",
	"Idle_7",
	"Idle_8",
	};

	constexpr std::array <engine::HashedStringView, 9> kMoveAnimNames = {
	"Move_0",
	"Move_1",
	"Move_2",
	"Move_3",
	"Move_4",
	"Move_5",
	"Move_6",
	"Move_7",
	"Move_8",
	};

	Idle::Idle()
		: Super(Idle::kClassConcreteName)
	{}
	Idle::~Idle()
	{}
	void Idle::OnAwake(const engine::AIContext& ai_context)
	{
		Super::OnAwake(ai_context);

		ASSERT(ai_context.black_board);

		playing_animation_ = ai_context.black_board->GetValue<engine::HashedStringView>("PlayingAnimation");
		ASSERT(playing_animation_);
	}
	void Idle::OnEnter(const engine::AIContext& ai_context)
	{
		Super::OnEnter(ai_context);
		//Idle에 진입했을 때 수행할 로직 작성
		DEBUG_LOG("Idle::OnEnter() called.");

		*playing_animation_ = "Idle";
	}
	void Idle::OnExit(const engine::AIContext& ai_context)
	{
		Super::OnExit(ai_context);

		DEBUG_LOG("Idle::OnExit() called.");
	}
	void Idle::OnUpdate(const engine::AIContext& ai_context)
	{
		Super::OnUpdate(ai_context);

	}
	engine::HashedStringView Idle::CheckTransition(const engine::AIContext& ai_context)
	{
		return "";
	}
}