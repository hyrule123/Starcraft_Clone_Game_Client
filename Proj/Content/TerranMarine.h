#pragma once
#include <Engine/Game/GameObject.h>

#include <Engine/Core/CoreMinimal.h>

namespace engine
{
    class Material;
    class TerranMarine :
        public GameObject
    {
        CLASS_INFO(TerranMarine, GameObject)
    public:
        TerranMarine();
        virtual ~TerranMarine() override;

        virtual void Init() override;

    private:
        s_ptr<Material> mtrl_ = {};
    };
}


