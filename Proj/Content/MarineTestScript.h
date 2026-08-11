#pragma once
#include <Engine/Game/Component/Script.h>

namespace engine
{
    class MarineTestScript :
        public Script
    {
		ENTITY_INFO(MarineTestScript, Script)
	public:
		MarineTestScript();
		virtual ~MarineTestScript() override;

		virtual void Init() override;

		virtual void OnCollisionEnter2D(const Collision2D& _info) override;
		virtual void OnCollisionExit2D(Collider2D* other) override;
    };
}

