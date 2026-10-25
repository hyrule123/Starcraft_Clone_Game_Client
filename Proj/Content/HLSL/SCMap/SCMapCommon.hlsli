#ifndef HLSL_SC_MAP_COMMON
#define HLSL_SC_MAP_COMMON

#include <Engine/HLSL/Core/CoreMinimal.hlsli>
#include <Content/HLSL/SCCommon.hlsli>


struct MapInfoCB
{
	int2 megatile_size;
	int2 padding;
	matrix world_mat;
};

#ifdef __HLSL

#endif//__HLSL

#endif //HLSL_SC_MAP_COMMON