#ifndef HLSL_SC_MAP_COMMON
#define HLSL_SC_MAP_COMMON

#include <Engine/HLSL/CppShared/CoreMinimal.hlsli>

#define MEGATILE_PIXEL_SIZE 32

struct alignas(16) MapInfoCB
{
	int2 megatile_size;
	uint2 padding;
};

#endif//HLSL_SC_MAP_COMMON