#ifndef SC_PCX_LUT_HLSLI
#define SC_PCX_LUT_HLSLI
#include <Content/HLSL/SCImage/SCImageCommon.hlsli>

SC_CONST uint kShadowDarkRow = 18; // OpenBW 하드 코딩된 항목
SC_CONST uint kPcxLutTableWidth = 256;

// Pcx LUT table indices
SC_CONST uint kPcxLutDarkIdx = 0;
SC_CONST uint kPcxLutOfireIdx = 1;
SC_CONST uint kPcxLutGfireIdx = 2;
SC_CONST uint kPcxLutBfireIdx = 3;
SC_CONST uint kPcxLutBexplIdx = 4;
SC_CONST uint kPcxLutTrans50Idx = 5;
SC_CONST uint kPcxLutRedIdx = 6;
SC_CONST uint kPcxLutGreenIdx = 7;
SC_CONST uint kPcxLutCount = 8;

#ifdef __HLSL
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

#endif//__HLSL

#endif//SC_PCX_LUT_HLSLI