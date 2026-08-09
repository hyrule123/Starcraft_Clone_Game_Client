#pragma once
#include <Engine/Game/GameObject.h>

#include <Engine/Core/CoreMinimal.h>

namespace engine
{
    class SpriteAnimClip;
    class SpriteAnimation;
    class Material;
    class TerranMarine :
        public GameObject
    {
        ENTITY_INFO(TerranMarine, GameObject)
    public:
        TerranMarine();
        virtual ~TerranMarine() override;

        virtual void Init() override;

    private:
        s_ptr<SpriteAnimation> anim_ = {};

        //읽기 전용 및 validation 예정, AnimationHandler에서 사용할 용도 
		static inline std::vector<SpriteAnimClip*> move_clips_per_direction_ = {};
		static inline std::vector<SpriteAnimClip*> idle_clips_per_direction_ = {};
		static inline std::vector<SpriteAnimClip*> attack_windup_clips_per_direction_ = {};
		static inline std::vector<SpriteAnimClip*> attack_clips_per_direction_  = {};
    };
}


