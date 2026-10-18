#include <Content/HLSL/SCImage/SCImageCommon.hlsli>

RasterizerOrderedTexture2D<uint> g_sc_index_rt : register(REG_U_SC_WPE_TO_ROV_SLOT);
Buffer<uint> g_pcx_tileset_lut : register(REG_T_SC_PCX_TILESET_LUT); // R8_UINT
Buffer<uint2> g_pcx_lut_ranges : register(REG_T_SC_PCX_TILESET_LUT_OFFSETS); // x = offset, y = rows

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
		case kModifierCloaked: // 은폐
		{
			// 텍스처에서 샘플링된 색(index)에 플레이어 색상 적용
			const uint src = ApplyPlayerColor(sampled.x, input.player_color_idx);
			
			// 샘플링된 색을 50% 투명 변환 테이블을 통해 변환, ROV에 기록
			g_sc_index_rt[pixel] = PcxLookupLight(kPcxLutTrans50Idx, src, old);
			break;
		}
		
		default:
		{
			g_sc_index_rt[pixel] = ApplyPlayerColor(sampled.x, input.player_color_idx);
			break;
		}
	}
}