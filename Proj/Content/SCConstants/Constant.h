#pragma once

#include <Engine/Core/Math.h>

namespace engine
{
	namespace SC
	{
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