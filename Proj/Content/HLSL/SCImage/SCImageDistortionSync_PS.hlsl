#include <Content/HLSL/SCImage/SCImageCommon.hlsli>
#include <Content/HLSL/SCImage/SCEffectPass.hlsli>

[earlydepthstencil]
void main(ImageVSOutput input)
{		
	SampleGrpOrDiscard(input);
	
	const uint2 pixel = uint2(input.position.xy);
	
	// WPE(A)에 쓴 결과를 Mirror(B)에 반영
	g_sc_mirror_rt[pixel] = g_sc_wpe_rt[pixel];
}