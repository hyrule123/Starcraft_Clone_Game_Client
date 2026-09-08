#ifndef HLSL_SC_TILE_SET
#define HLSL_SC_TILE_SET

#include <Engine/HLSL/Core/CoreMinimal.hlsli>

#include <Content/HLSL/SCMapCommon.hlsli>

// 맵 정보 레지스터: SCMapCommon.hlsli에서 정의됨(REG_B_MAP_INFO)

// 타일셋 리소스
#define REG_T_TILESET_CV5 REG_T_PER_DRAW_0
#define REG_T_TILESET_VX4 REG_T_PER_DRAW_1
#define REG_T_TILESET_VR4 REG_T_PER_DRAW_2
// WPE는 이 패스에서 사용하지 않음(결과가 WPE index)

// 맵 리소스
#define REG_T_MAP_MTXM REG_T_PER_DRAW_3

// 아웃풋
#define REG_U_MAP_TEXTURE REG_U(0)

#ifdef  __HLSL

cbuffer cbuffer_map_info : register(REG_B_MAP_INFO)
{
	MapInfoCB g_map_info;
};

// 조회 체인:
//   MTXM  → megatile 인덱스
//   CV5   → VX4 인덱스
//   VX4   → VR4 인덱스 + flip (bit0 = flip)
//   VR4   → WPE 인덱스
//   WPE   → RGB               (이 패스에서는 사용 안 함)

// CV5에서 Megatile Index만 뽑아서 평탄화한 버퍼 (Megatile index group = 16개)
// CV5_megatiles[megatile flat index] = VX4_index
Buffer<uint> CV5_megatiles : register(REG_T_TILESET_CV5);

// VX4는 한 그룹당 uint16 4 * 4개 = 16개
// bit0=flip, bit1~ = VR4 인덱스
// VX4_minitiles[VX4_index] = minitile 4 * 4 정보(0 ~ 15) = flip & VR4_index
Buffer<uint> VX4_minitiles : register(REG_T_TILESET_VX4); 

// VR4는 한 그룹당 uint8 8 * 8개 = 64개 (메가타일의 픽셀 WPE 정보)
// VR4_minitile_wpe_indices[VR4_index * 64 + p] = WPE_index
Buffer<uint> VR4_minitile_wpe_indices : register(REG_T_TILESET_VR4);

// MTXM_CV5_indices[tile_y * map_tile_width + tile_x] = megatile flat index
Buffer<uint> MTXM_CV5_indices : register(REG_T_MAP_MTXM);

// output
RWTexture2D<uint> map_output : register(REG_U_MAP_TEXTURE);

#endif//__HLSL

#endif//HLSL_SC_TILE_SET