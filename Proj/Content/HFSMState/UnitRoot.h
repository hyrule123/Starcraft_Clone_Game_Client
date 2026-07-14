#pragma once
#include <Engine/Game/Component/HFSMState.h>

namespace engine
{
	class UnitRoot :
		public HFSMState
	{
		CLASS_INFO(UnitRoot, HFSMState)
	public:
		UnitRoot();
		virtual ~UnitRoot() override;

		virtual void OnAwake(const AIContext& ai_context) override;
	};
}


