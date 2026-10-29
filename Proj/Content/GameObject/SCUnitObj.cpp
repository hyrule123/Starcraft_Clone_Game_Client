#include "Content/pch.h"
#include "SCUnitObj.h"

#include <Engine/Core/Debug.h>

#include <Content/Script/SCUnit.h>

#include <SCSimulator/Include/SCSimulatorAPI.h>

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

