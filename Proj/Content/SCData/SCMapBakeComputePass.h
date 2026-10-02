#pragma once
#include <Engine/Resource/GPU/Shader/ComputePass.h>

#include <Content/SCData/SCMapCommon.h>

struct ID3D11DeviceContext;

namespace sc
{	
    class SCMapBakeComputePass :
        public engine::ComputePass
    {
		ENTITY_INFO(SCMapBakeComputePass, engine::ComputePass)
    public:
        SCMapBakeComputePass();
		virtual ~SCMapBakeComputePass() override;
		
		virtual std::shared_ptr<engine::ComputeShader> LoadComputeShader() override;

		std::shared_ptr<engine::Texture2D> BakeMapTexture(const SCMapLoadingData& map_data);

	protected:
		virtual void BindResources(ID3D11DeviceContext* context) override;
		virtual void UnbindResources(ID3D11DeviceContext* context) override;
		virtual std::array<UINT, 3> GetThreadCount() const override;

	private:
		std::unique_ptr<engine::ConstantBuffer> map_info_buffer_ = {};
		
		std::array<UINT, 3> thread_count_ = { 0, 0, 0 };
    };
}


