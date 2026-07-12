#pragma once
#include <Engine/Game/GameObject.h>

#include <Engine/Core/CoreMinimal.h>

namespace engine
{
    class SpriteAnimation;
    class Material;
    class TerranMedic :
        public GameObject
    {
        CLASS_INFO(TerranMedic, GameObject)
    public:
        TerranMedic();
        virtual ~TerranMedic() override;

        virtual void Init() override;

    private:
        s_ptr<SpriteAnimation> anim_ = {};
    };
}


