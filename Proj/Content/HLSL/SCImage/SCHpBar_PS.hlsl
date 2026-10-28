#include <Content/HLSL/SCImage/SCHpBar.hlsli>

uint main(HpBarVSOutput input) : SV_TARGET
{
	/*
	실드 유무에 따라 hp 행 5행 또는 7행, 쉴드 행 0~3은 덮어쓰기
	에너지 행은 5행이고, hp 영역과의 사이 1행은 discard
	채움 폭 안쪽은 단계 색, 바깥은 배경 색으로 하고 g_common_palettes[kPaletteTHpBarOffset + ci]로 조회.
	*/
	
	const int2 pixel = int2(floor(input.local));
	
	const uint x = uint(pixel.x);
	const uint y = uint(pixel.y);
	const bool has_shield = (input.bar_flags & kHpBarFlagShield) != 0;
	const uint hp_h = kHpBarBaseHeight + (has_shield ? kHpBarShieldExtraHeight : 0);

	uint ci = 0;
	if (has_shield && y < kHpBarShieldRowCount)						// 쉴드 (HP 블록 위를 덮음)
	{
		ci = (x < input.shield_fill) ? kShieldColors[kShieldColorsFillRow][y] : kShieldColors[kShieldColorsBgRow][y];
	}
	else if (y < hp_h)								// HP
	{
		const uint row = (x < input.hp_fill) ? input.hp_stage : kHpColorsBgRow;
		ci = has_shield ? kHpColorsWithShield[row][y] : kHpColorsNoShield[row][y];
	}
	else if (y == hp_h)								// 간격 (에너지 있을 때만 존재)
	{
		discard;
	}
	else // 에너지
	{
		const uint e = y - (hp_h + 1);
		ci = (x < input.energy_fill) ? kEnergyColors[kEnergyColorsFillRow][e] : kEnergyColors[kEnergyColorsBgRow][e];
	}

	// 3픽셀 구분선: 채움/배경 상관없이 전 구간 (간격 행은 위에서 discard됨)
	if (x % 3 == 0)
	{
		ci = kHpBarBorderColor;
	}

	return g_common_palettes[kPaletteTHpBarOffset + ci];
}