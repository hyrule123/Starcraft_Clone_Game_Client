#include "Content/pch.h"
#include "MarineTestScript.h"

#include <Engine/Core/Debug.h>

namespace engine
{
	MarineTestScript::MarineTestScript()
		: Super(MarineTestScript::kClassConcreteName)
	{}

	MarineTestScript::~MarineTestScript()
	{}

	void MarineTestScript::Init()
	{
		Super::Init();

		Subscribe(SubscribeType::kCollision);
	}
	void MarineTestScript::OnCollisionEnter2D(const Collision2D& _info)
	{
		Super::OnCollisionEnter2D(_info);

		DEBUG_LOG("MarineTestScript::OnCollisionEnter2D()");
	}
	void MarineTestScript::OnCollisionExit2D(Collider2D * other)
	{
		Super::OnCollisionExit2D(other);

		DEBUG_LOG("MarineTestScript::OnCollisionExit2D()");
	}
}

