#include <Content/HLSL/SCImage/SCEffectPass.hlsli>
#include <Content/HLSL/SCImage/SCPcxLut.hlsli>

// Effect 패스 계약: 모든 modifier는 WriteIndex()를 통해 ROV에 기록해야 함
// (Distortion Read가 B를 '드로우 직전의 A'로 간주하고 읽기 때문)
void WriteIndex(uint2 pixel, uint value)
{
	g_sc_wpe_rt[pixel] = value;
	g_sc_mirror_rt[pixel] = value;
}

[earlydepthstencil]
void main(ImageVSOutput input)
{
	const uint2 sampled = SampleGrpOrDiscard(input);
	
	const uint2 pixel = uint2(input.position.xy);
	
	//현재 픽셀의 인덱스 RT 값(old) 가져오기
	const uint old = g_sc_wpe_rt[pixel];
	uint new_val = 0;
	
	switch(input.modifier)
	{
		case kModifierShadow:	// 그림자
			{
				new_val = PcxLookup(kPcxLutDarkIdx, kShadowDarkRow, old);
				break;
			}
		
		// 둘은 코드가 동일(방향만 다름)
		case kModifierCloaking: // 은폐 중
		case kModifierDecloaking: // 은폐 해제 중
			{
				// 텍스처에서 샘플링된 색(index)에 플레이어 색상 적용
				const uint src = ApplyPlayerColor(sampled.x, input.player_color_idx);
				const uint blended = PcxLookupLight(kPcxLutTrans50Idx, src, old);
				
				// 사용 기준 판별은 '적용 전 색상' 기준
				const bool is_cloaked = g_color_fade_selector[sampled.x] <= (uint) input.modifier_data1;
			
				new_val = is_cloaked ? blended : src;
				break;
			}
		
		case kModifierCloaked: // 은폐
			{
				// 텍스처에서 샘플링된 색(index)에 플레이어 색상 적용
				const uint src = ApplyPlayerColor(sampled.x, input.player_color_idx);

				// 샘플링된 색을 50% 투명 변환 테이블을 통해 변환, ROV에 기록
				new_val = PcxLookupLight(kPcxLutTrans50Idx, src, old);
				break;
			}
		
		case kModifierRemapping:
			{
				// remapping이 유효한 범위인지 확인
				const bool is_valid = kPcxLutOfireIdx <= input.remapping && input.remapping < kPcxLutCount;
				if(is_valid)
				{	
					// OpenBW no_remap: 플레이어 색 치환 없이 원본 인덱스 사용
					new_val = PcxLookupLight(input.remapping, sampled.x, old);
				}
				else
				{
					new_val = ApplyPlayerColor(sampled.x, input.player_color_idx);
				}
				
				break;
			}
		
		case kModifierWarpFlash:
			{
				new_val = PcxLookupLight(kPcxLutOfireIdx, (uint) input.modifier_data1, old);
				break;
			}
		
		default:
			{
				new_val = ApplyPlayerColor(sampled.x, input.player_color_idx);
				break;
			}
	}
	
	WriteIndex(pixel, new_val);
}