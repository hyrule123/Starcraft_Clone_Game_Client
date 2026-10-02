#include "Content/pch.h"
#include "SCUnitObj.h"

#include <Content/Script/SCUnit.h>
#include <Content/Legacy/Script/SCUnitBehavior.h>

#include <Engine/Core/Debug.h>

namespace sc
{
	SCUnitObj::SCUnitObj()
		: Super(SCUnitObj::kClassConcreteName)
	{}

	SCUnitObj::~SCUnitObj()
	{}

	void SCUnitObj::Init()
	{
		Super::Init();

		sc_unit_ = AddComponent<SCUnit>();
		sc_unit_behavior_ = AddComponent<SCUnitBehavior>();

		CHECK(sc_unit_ && sc_unit_behavior_);
	}


}

