#pragma once
#include <Engine/Game/Component/Script.h>

#include <Engine/Core/Geometry.h>

namespace engine
{
    class Camera;
}

namespace sc
{
    class CameraMoveScript :
        public engine::Script
    {
        ENTITY_INFO(CameraMoveScript, engine::Script);

	public:
        CameraMoveScript();
        virtual ~CameraMoveScript() override;
        
        virtual void Awake() override;
        virtual void Update() override;

		void SetClampBounds(const engine::AABB2D& bounds) { 
            sc_clamp_bounds_ = bounds; 
            ClampCameraPosition(); 
        }
        void SetClampViewSize(float2 view_size) { 
            CHECK_F(view_size.x > 0 && view_size.y > 0, "Invalid clamp view size: ({}, {})", view_size.x, view_size.y); 
            view_size_ = view_size; 
            ClampCameraPosition();
        }

    private:
		void ClampCameraPosition();

    private:
        engine::weak_handle_ptr<engine::Camera> cam_ = {};

		float2 sc_cam_pos_ = {};
        float speed_ = {};
        const float pos_update_interval_ = {};
        float pos_update_timer_ = {};

		// min은 0/0, max는 맵의 최대 좌표. (SC 좌표계)
		engine::AABB2D sc_clamp_bounds_ = {};
        float2 view_size_ = {};   // View의 사이즈. 현재는 외부로부터 세팅하지 않고, 상수로 지정하고 있음. 나중에 필요 시 함수로 확장
    };
}


