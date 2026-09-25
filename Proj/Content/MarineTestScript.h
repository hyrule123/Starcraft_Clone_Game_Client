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
		virtual void Awake() override;
		virtual void OnEnable() override;
		virtual void Update() override;
		virtual void FixedUpdate() override;
		virtual void LateUpdate() override;
		virtual void OnDisable() override;
		virtual void OnDestroy() override;

		virtual void OnCollisionEnter2D(const Collision2D& _info) override;
		virtual void OnCollisionExit2D(Collider2D* other) override;
    };
}

