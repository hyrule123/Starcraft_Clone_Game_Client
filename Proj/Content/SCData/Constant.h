#pragma once

#include <Engine/Core/StdType.h>
#include <Engine/Core/Math.h>

namespace engine
{
	namespace SC
	{
		constexpr uint32 kResolutionWidth = 640;
		constexpr uint32 kResolutionHeight = 480;

		constexpr float2 kCellSize = float2(32.0f * 2.0f, 32.0f * 2.0f);	//CollisionSystem2D에서 사용하는 Cell Size

		constexpr float kFPS = 23.81f;
		constexpr float kFrameTime = 1.0f / kFPS;
		constexpr float kDegree = 256.0f;	//내부적으로 256단계로 분할

		//Degree Per Second = 40 -> 초당 40 / 256만큼을 회전함
		constexpr float DegreePerSecond(float sc_degree_per_sec)
		{
			return (sc_degree_per_sec / kDegree) * 360.0f * kFrameTime;
		}
		constexpr float RadianPerSecond(float sc_degree_per_sec)
		{
			return (sc_degree_per_sec / kDegree) * kTwoPi * kFrameTime;
		}
	}

}