#pragma once

#include <Engine/Game/Scene.h>

namespace engine
{
    class TestScene :
        public Scene
    {
    public:
        TestScene();
        virtual ~TestScene() override;

        virtual void Init() override;

        virtual void Render() override;

    private:
        s_ptr<class InputLayout> il_ = {};
        s_ptr<class VertexShader> vs_ = {};
        s_ptr<class PixelShader> ps_ = {};
        s_ptr<class Mesh> msh_ = {};
    };
}


