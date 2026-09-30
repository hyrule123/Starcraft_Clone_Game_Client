#pragma once
#include <Engine/Game/Component/Script.h>

namespace sc
{
    class MarineTestScript :
        public engine::Script
    {
		ENTITY_INFO(MarineTestScript, engine::Script)
	public:
		MarineTestScript();
		virtual ~MarineTestScript() override;

		virtual void Init() override;
		virtual void Awake() override;
		virtual void OnEnable() override;
		virtual void Update() override;
		virtual void FixedUpdate() override;
		virtual void LateUpdate() override;
		virtual void OnDisable() override;
		virtual void OnDestroy() override;

		virtual void OnCollisionEnter2D(const engine::Collision2D& _info) override;
		virtual void OnCollisionExit2D(engine::Collider2D* other) override;
    };
}

