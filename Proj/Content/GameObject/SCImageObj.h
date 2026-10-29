#pragma once
#include <Engine/Game/GameObject.h>

#include <Content/SCData/Image/SCSpriteCommon.h>

#include <SCSimulator/Include/SCSimulatorTypes.h>

namespace sc
{
    class SCImageRenderer;

	//설명: SCSpriteObj의 자식으로 붙어서 실제 이미지를 표시하는 게임 오브젝트(Renderer 부착)
    class SCImageObj :
        public engine::GameObject
    {
		ENTITY_INFO(SCImageObj, engine::GameObject);

    public:
        SCImageObj();
        virtual ~SCImageObj() override;

		virtual void Init() override;

		void ApplySnapshot(const SCSim::ImageData& img_data, const SCImageParam& img_param);

    private:
		SCSim::ImageData image_data_ = {};

        SCImageRenderer* renderer_ = {};
    };
}


