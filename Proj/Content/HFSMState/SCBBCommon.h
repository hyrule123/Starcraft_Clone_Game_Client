#pragma once
#include <Engine/Core/CoreMinimal.h>

// 설명: Blackboard에서 사용될 일반적인 데이터

namespace engine
{
	class BlackBoard;

	namespace SC::black_board
	{
		constexpr HashedStringView kCommandInput = "CommandInput"_hash;
		constexpr HashedStringView kCurrentAction = "CurrentAction"_hash;
		constexpr HashedStringView kDestination = "Destination"_hash;
		constexpr HashedStringView kDirection = "Direction"_hash;
		constexpr HashedStringView kPlayingAnimation = "PlayingAnimation"_hash;
		constexpr HashedStringView kSpeed = "Speed"_hash;
		constexpr HashedStringView kIsMoving = "IsMoving"_hash;
		constexpr HashedStringView kIsSCDataSet = "IsSCDataSet"_hash;
		constexpr HashedStringView kTurnSpeed = "TurnSpeed"_hash;

		// BlackBoard에서 사용하는 데이터가 있는지 확인하고, 없을 시 등록한다.
		void EnsureBlackBoardData(BlackBoard* blackboard);
	}
	
}