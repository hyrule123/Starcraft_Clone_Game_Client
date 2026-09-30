#pragma once
#include <Engine/Core/CoreMinimal.h>

// 설명: Blackboard에서 사용될 일반적인 데이터

namespace engine
{
	class BlackBoard;

}
namespace sc::black_board
{
	constexpr engine::HashedStringView kCommandInput = "CommandInput";
	constexpr engine::HashedStringView kCurrentAction = "CurrentAction";
	constexpr engine::HashedStringView kDestination = "Destination";
	constexpr engine::HashedStringView kDirection = "Direction";
	constexpr engine::HashedStringView kPlayingAnimation = "PlayingAnimation";
	constexpr engine::HashedStringView kSpeed = "Speed";
	constexpr engine::HashedStringView kIsMoving = "IsMoving";
	constexpr engine::HashedStringView kIsSCDataSet = "IsSCDataSet";
	constexpr engine::HashedStringView kTurnSpeed = "TurnSpeed";

	// BlackBoard에서 사용하는 데이터가 있는지 확인하고, 없을 시 등록한다.
	void EnsureBlackBoardData(engine::BlackBoard* blackboard);
}