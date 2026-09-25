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

		DEBUG_LOG("MarineTestScript::Init() called.");
	}
	void MarineTestScript::Awake()
	{
		Super::Awake();
		DEBUG_LOG("MarineTestScript::Awake() called.");
	}
	void MarineTestScript::OnEnable()
	{
		Super::OnEnable();
		DEBUG_LOG("MarineTestScript::OnEnable() called.");
	}
	void MarineTestScript::Update()
	{
		Super::Update();
		static bool  is_first_update = false;
		if (is_first_update == false)
		{
			DEBUG_LOG("MarineTestScript::Update() called.");
			is_first_update = true;
		}
	}
	void MarineTestScript::FixedUpdate()
	{
		Super::FixedUpdate();
		static bool  is_first_fixed_update = false;
		if (is_first_fixed_update == false)
		{
			DEBUG_LOG("MarineTestScript::FixedUpdate() called.");
			is_first_fixed_update = true;
		}
	}
	void MarineTestScript::LateUpdate()
	{
		Super::LateUpdate();
		static bool  is_first_late_update = false;
		if (is_first_late_update == false)
		{
			DEBUG_LOG("MarineTestScript::LateUpdate() called.");
			is_first_late_update = true;
		}
	}
	void MarineTestScript::OnDisable()
	{
		Super::OnDisable();
		DEBUG_LOG("MarineTestScript::OnDisable() called.");
	}
	void MarineTestScript::OnDestroy()
	{
		Super::OnDestroy();
		DEBUG_LOG("MarineTestScript::OnDestroy() called.");
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

