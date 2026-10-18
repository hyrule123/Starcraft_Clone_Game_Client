#ifndef HLSL_SC_MAP_COMMON
#define HLSL_SC_MAP_COMMON

#include <Engine/HLSL/Core/CoreMinimal.hlsli>
#include <COntent/HLSL/SCCommon.hlsli>

#define REG_B_MAP_INFO		REG_B_PER_PIPELINE_0


#define MEGATILE_PIXEL_SIZE 32

struct MapInfoCB
{
	int2 megatile_size;
	int2 padding;
	matrix world_mat;
};

#endif//HLSL_SC_MAP_COMMON