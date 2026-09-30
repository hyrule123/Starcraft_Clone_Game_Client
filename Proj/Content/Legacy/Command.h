#pragma once
#include <Engine/Core/Math.h>

#include <variant>

namespace sc
{
	struct MoveCommand
	{
		float2 destination = {};
	};

	using Command = std::variant<std::monostate, MoveCommand>;
