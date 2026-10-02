#pragma once
#include <Engine/Game/GameObject.h>

#include <SCSimulator/Include/SCSimulatorAPI.h>

namespace sc
{
	//설명: SCImageObj의 루트 클래스. 공통 로직 데이터(WorldPos 등...) 저장용
    // Renderer 미부착, 직접 렌더링을 하지 않는다.
	// 실제 이미지 렌더링은 SCImageObj가 담당 - 자식 클래스로 SCImageData를 가진다.
    class SCSpriteObj :
        public engine::GameObject
    {
        ENTITY_INFO(SCSpriteObj, engine::GameObject);

	public:
        SCSpriteObj();
		virtual ~SCSpriteObj() override;

        void ApplySnapshot(const SCSim::SpriteData& sprite_data);
    };
}


