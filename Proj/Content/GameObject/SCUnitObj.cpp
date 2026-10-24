#include "Content/pch.h"
#include "SCUnitObj.h"

#include <Content/Script/SCUnit.h>

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
	}
}

