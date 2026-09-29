#ifndef HLSL_SC_MAP_COMMON
#define HLSL_SC_MAP_COMMON

#include <Engine/HLSL/Core/CoreMinimal.hlsli>

#define REG_B_MAP_INFO		REG_B_PER_PIPELINE_0
#define REG_T_SC_TUNIT			REG_T_PER_FRAME_5
#define REG_T_SC_WPE_PALETTE	REG_T_PER_FRAME_6
#define REG_T_SC_PCX_TILESET_LUT	REG_T_PER_FRAME_7


#define MEGATILE_PIXEL_SIZE 32

struct MapInfoCB
{
	int2 megatile_size;
	int2 padding;
	matrix world_mat;
};

#endif//HLSL_SC_MAP_COMMON