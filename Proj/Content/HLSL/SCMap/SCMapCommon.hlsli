#ifndef HLSL_SC_MAP_COMMON
#define HLSL_SC_MAP_COMMON

#include <Engine/HLSL/Core/CoreMinimal.hlsli>
#include <Content/HLSL/SCCommon.hlsli>


struct MapInfoCB
{
	int2 map_size_megatile;
	int2 padding;
	matrix world_mat;
};

// 맵 로딩 관련 상수
HLSL_CONSTEXPR uint kMTXMGroupShift = 4;
HLSL_CONSTEXPR uint kMTXMGroupMask = 0x7FF;
HLSL_CONSTEXPR uint kMTXMSubtileMask = 0xF;
HLSL_CONSTEXPR uint kMTXMTileIdMask = 0x7FFF;

#ifdef __HLSL

#endif//__HLSL

#endif //HLSL_SC_MAP_COMMON