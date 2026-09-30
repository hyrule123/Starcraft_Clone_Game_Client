#pragma once
#include <Engine/Game/Component/HFSMState.h>

namespace sc
{
	class UnitRoot :
		public engine::HFSMState
	{
		ENTITY_INFO(UnitRoot, engine::HFSMState)
	public:
		UnitRoot();
		virtual ~UnitRoot() override;

		virtual void OnAwake(const engine::AIContext& ai_context) override;

		virtual engine::HashedStringView CheckTransition(const engine::AIContext& ai_context) override;

	private:
		engine::HashedStringView* command_input_ = {};
		engine::HashedStringView* current_action_ = {};
	};
}


