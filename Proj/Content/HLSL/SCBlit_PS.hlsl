#include <Engine/HLSL/BlitPass/BlitPass.hlsli>

#include <Content/HLSL/SCImage/SCImageCommon.hlsli>

Texture2D<uint> g_src_wpe_index : register(REG_T_SRC_RENDERTARGET);

float4 main(BlitVSOutput input) : SV_TARGET
{
	// 입력(인덱스 RT)과 출력이 같은 크기(1:1)라 SV_Position의 픽셀 좌표를 그대로 사용
	const int2 texel = int2(input.position.xy);
	const uint index = g_src_wpe_index.Load(int3(texel, 0));

	// WPE 네 번째 바이트는 패딩이라 알파로 쓰지 않음
	return float4(g_wpe[index].rgb, 1.0);
}