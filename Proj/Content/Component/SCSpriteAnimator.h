#pragma once
#include <Engine/Game/Component/Component.h>

namespace engine
{
    class SCSpriteRenderer;
    class SpriteAnimation;
    class SpriteAnimClip;

    class SCSpriteAnimator :
        public Component
    {
        CLASS_INFO(SCSpriteAnimator, Component)
            COMPONENT_CATEGORY(ComponentCategory::kAnimator)
    public:
        SCSpriteAnimator();
        virtual ~SCSpriteAnimator() override;

        virtual void Init() override;
        virtual void Awake() override;

        virtual void LateUpdate() override;

        bool SetSpriteAnimation(const HashedStringView& res_path);
        void SetSpriteAnimation(s_ptr<SpriteAnimation> anim) { anim_ = anim; }

        bool Play(const HashedStringView& anim_name);
        bool Play(SpriteAnimClip* clip_ptr);

    private:
        w_ptr<SCSpriteRenderer> renderer_ = {};
        s_ptr<SpriteAnimation> anim_ = {};

        SpriteAnimClip* playing_clip_ = {};

        bool is_playing_ = {};

        //상태 캐시
        float acc_deltatime_ = {};
        uint32 cur_frame_idx_ = {};
        uint32 clip_frame_total_count_ = {};
        float cur_frame_duration_ = {};
    };
}


