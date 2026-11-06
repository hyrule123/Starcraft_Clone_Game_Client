#pragma once
#include <Engine/Game/Scene.h>

namespace sc
{
    class SceneTransitionTestScene :
        public engine::Scene
    {
		ENTITY_INFO(SceneTransitionTestScene, engine::Scene);

    public:
        SceneTransitionTestScene();
		virtual ~SceneTransitionTestScene() override;

        virtual void Init() override;
        
    protected:
        virtual void OnFrameStart() override;
    };
}


