#include <Content/HLSL/SCImage/SCImageCommon.hlsli>
#include <Content/HLSL/SCImage/SCEffectPass.hlsli>
#include <Content/HLSL/SCImage/SCPcxLut.hlsli>	// g_color_fade_selector

static const uint kDebugIndexA = 0;
static const uint kDebugIndexB = 255;

// Distortion: B에서 오른쪽 n픽셀 위치를 읽어 A에만 기록 (B는 Sync에서 반영)
void ApplyDistortion(uint2 pixel, uint n)
{
	const uint local_x = pixel.x - g_sc_common_info.view_origin.x;
	const bool in_view = (local_x + n) < g_sc_common_info.view_size.x;

	// 뷰 오른쪽 끝을 넘으면 원래 값 유지 (행 넘김·가장자리 복제 대신)
	const uint2 src_pixel = in_view ? uint2(pixel.x + n, pixel.y) : pixel;

	g_sc_wpe_rt[pixel] = g_sc_mirror_rt[src_pixel];
}

[earlydepthstencil]
void main(ImageVSOutput input)
{
	// GRP의 샘플링된 값, 아직 이 값의 의미는 알 수 없음(modifier 확정 시 알 수 있음)
	const uint2 sampled = SampleGrpOrDiscard(input);
	
	const uint2 pixel = uint2(input.position.xy);
	
	switch (input.modifier)
	{
		// 둘은 코드가 동일(방향만 다름)
		case kModifierEnemyCloaking: // 은폐 중
		case kModifierEnemyDecloaking: // 은폐 해제 중
			{
				// 텍스처에서 샘플링된 색(index)에 플레이어 색상 적용
				const uint src = ApplyPlayerColor(sampled.x, input.player_color_idx);
				
				// 사용 기준 판별은 '적용 전 색상' 기준
				const bool is_cloaked = g_color_fade_selector[sampled.x] <= (uint) input.modifier_data1;
				
				// cloak된 픽셀이면 Distortion 적용(왜곡 + 투명화), 아니면 기존 색상 적용
				if(is_cloaked)
				{
					ApplyDistortion(pixel, g_color_fade_selector[src]);
				}
				else
				{
					g_sc_wpe_rt[pixel] = src;
				}
				
				break;
			}
		
		// 탐지 못 한 적의 은폐: 밝기 등급(cloak_fade_selector, 0~8)을 오프셋으로 사용
		// 오프셋은 플레이어 색 치환 후 인덱스의 밝기 등급. (혹시나 원작과 일치하지 않을 경우 이 부분을 수정)
		// 5/7과 같은 기준을 유지할 것
		case kModifierEnemyCloaked:
			{
				const uint src = ApplyPlayerColor(sampled.x, input.player_color_idx);
				ApplyDistortion(pixel, g_color_fade_selector[src]);
				break;
			}
		
		// Distortion GRP: 기록된 값 만큼 오른쪽 픽셀의 값을 가져와서 기록한다.
		case kModifierDistortion:
			{
				ApplyDistortion(pixel, sampled.x);
				break;
			}
		
		// 여기 오면 안 됨: Distortion Material에 다른 modifier가 들어온 경우 (GetMaterialVariant 매핑 오류)
		// 팔레트와 무관하게 눈에 띄도록 체커 패턴으로 기록
		default:
			{
				g_sc_wpe_rt[pixel] = ((pixel.x ^ pixel.y) & 1) ? kDebugIndexA : kDebugIndexB;
				break;
			}
	}
}


/*
[LOG] [Distortion] image 555 -> source 436 (tbl 531) unit\thingy\emp.grp
[LOG] [GRP dump] unit\thingy\emp.grp — 252x252, 프레임 7, 불투명 픽셀 35177, 값 범위 [1, 7]
[LOG] [GRP dump] 분포(값:개수) 1:17297 2:7662 3:4199 4:3781 5:877 6:875 7:486
[LOG] [Distortion] GRP 1개, 다른 draw_function과 공유하는 이미지 0개
[LOG] [GRP search] source 109 unit\protoss\dtemplar.grp
[LOG] [GRP dump] unit\protoss\dtemplar.grp — 64x64, 프레임 306, 불투명 픽셀 164924, 값 범위 [8, 255]
[LOG] [GRP dump] 분포(값:개수) 8:1730 9:1616 10:2551 11:3689 12:5211 13:7319 14:8324 15:794 16:4883 17:4 20:28 22:34 25:430 29:10 33:4 34:141 44:3 62:818 63:373 64:9085 65:13124 66:10960 67:2281 68:4283 69:857 70:2950 71:1011 72:2163 73:1321 74:788 75:311 78:373 81:439 83:215 84:90 85:746 86:329 87:996 89:1 90:1827 91:2234 93:249 95:34 96:4 99:493 100:2 102:112 105:6 106:2 107:766 110:46 112:2 114:388 118:6 119:221 124:17 125:8561 129:4 130:436 131:417 132:226 133:558 134:307 137:752 138:8504 139:9417 140:9227 141:11230 142:4327 143:1588 144:1690 145:1464 146:1285 147:421 148:1245 149:1698 150:915 151:924 152:462 153:441 154:839 155:1030 255:262

----- 위 내용을 바탕으로 추론한 것 -----
* Distortion을 명백하게 사용하는 Grp는 1개 뿐(emp.grp)
* 그런데, 실제 게임 렌더링 시 보이지 않는 클로킹 상태인 적도 Distortion을 사용하는 것 같음(왜곡 효과가 있는 듯 함)
    -> Distortion을 적용하는 것 아닌가?(합리적 의심)
* Distortion은 Grp에 저장된 값만큼 오른쪽 픽셀 값을 가져와서 왜곡을 연출함
* Distortion을 명백하게 사용하고 있는 emp.grp는, 범위가 1 - 7임. 많아야 오른쪽 7칸을 참조해서 왜곡 효과를 나타내고 있음.
* 하지만, Dark Templar의 색상값이 들어있는 Grp는 범위가 8 - 255임. 이걸 그대로 왜곡 효과에 그대로 반영하기에는 값 편차가 너무 큼.
* 실제 게임에서 육안으로 확인 시에도, 그 정도의 왜곡은 아니었음, 많아야 몇 칸 정도 오른쪽 값을 가져오는 듯한 느낌.

* 여기서부터 합리적 의심
** 클로킹 중간 과정을 묘사하는 데 사용되는 'fade selector'라는 값이 있는데, 이 범위가 0 - 8임. -> 비슷한 범위
** Grp 값을 'fade selector'에 넣어서, 0부터 8 사이의 값으로 변환하는 게 아닌가?
** 차후에 '왜곡 효과'를 구현한 뒤 실제로 테스트해보려고 함.
*/