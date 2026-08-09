#pragma once
#include <Engine/Game/Component/HFSMState.h>

namespace engine
{
	class UnitRoot :
		public HFSMState
	{
		ENTITY_INFO(UnitRoot, HFSMState)
	public:
		UnitRoot();
		virtual ~UnitRoot() override;

		virtual void OnAwake(const AIContext& ai_context) override;

		virtual HashedStringView CheckTransition(const AIContext& ai_context) override;

	private:
		HashedStringView* command_input_ = {};
		HashedStringView* current_action_ = {};
	};
}


