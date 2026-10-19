#include <Content/HLSL/SCImage/SCImageCommon.hlsli>

RasterizerOrderedTexture2D<uint> g_sc_index_rt : register(REG_U_SC_WPE_TO_ROV_SLOT);
Buffer<uint> g_pcx_tileset_lut : register(REG_T_SC_PCX_TILESET_LUT); // R8_UINT
Buffer<uint2> g_pcx_lut_ranges : register(REG_T_SC_PCX_TILESET_LUT_OFFSETS); // x = offset, y = rows

Buffer<uint> g_color_fade_selector : register(REG_T_SC_CLOAK_FADE_SELECTOR); // R8_UINT

// 원시 조회: table의 row행, old열
uint PcxLookup(uint table, uint row, uint old)
{
	const uint2 range = g_pcx_lut_ranges[table];
	return g_pcx_tileset_lut[range.x + row * kPcxLutTableWidth + old];
}

// light 계열 규칙 (OpenBW draw_alpha): row = src - 1 (uint8 감쌈), 범위 밖이면 인덱스 0
uint PcxLookupLight(uint table, uint src, uint old)
{
	const uint row = (src - 1) & 0xFF;
	uint result = 0; // 범위 밖이면 인덱스 0 (OpenBW 규칙)
	if (row < g_pcx_lut_ranges[table].y)
	{
		result = PcxLookup(table, row, old);
	}
	return result;
}

[earlydepthstencil]
void main(ImageVSOutput input)
{
	const int2 texel = int2(floor(input.local));
	
	// 텍스처에서 샘플링된 색(index)
	const uint2 sampled = g_grp_sprite.Load(int4(texel, input.frame, 0));

	// sampled.x = wpe index
	// sampled.y = 커버리지
	// 커버리지 0 = 투명
	if (sampled.y == 0)
	{
		discard; 
	}
	
	const uint2 pixel = uint2(input.position.xy);
	
	//현재 픽셀의 인덱스 RT 값(old) 가져오기
	const uint old = g_sc_index_rt[pixel];
	
	switch(input.modifier)
	{
		case kModifierShadow:	// 그림자
			{
				g_sc_index_rt[pixel] = PcxLookup(kPcxLutDarkIdx, kShadowDarkRow, old);
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
			
				g_sc_index_rt[pixel] = is_cloaked ? blended : src;
				break;
			}
		
		case kModifierCloaked: // 은폐
			{
				// 텍스처에서 샘플링된 색(index)에 플레이어 색상 적용
				const uint src = ApplyPlayerColor(sampled.x, input.player_color_idx);

				// 샘플링된 색을 50% 투명 변환 테이블을 통해 변환, ROV에 기록
				g_sc_index_rt[pixel] = PcxLookupLight(kPcxLutTrans50Idx, src, old);
				break;
			}
		
		case kModifierRemapping:
			{
				// remapping이 유효한 범위인지 확인
				const bool is_valid = kPcxLutOfireIdx <= input.remapping && input.remapping < kPcxLutCount;
				if(is_valid)
				{	
					// OpenBW no_remap: 플레이어 색 치환 없이 원본 인덱스 사용
					g_sc_index_rt[pixel] = PcxLookupLight(input.remapping, sampled.x, old);
				}
				else
				{
					g_sc_index_rt[pixel] = ApplyPlayerColor(sampled.x, input.player_color_idx);
				}
				
				break;
			}
		
		default:
			{
				g_sc_index_rt[pixel] = ApplyPlayerColor(sampled.x, input.player_color_idx);
				break;
			}
	}
}