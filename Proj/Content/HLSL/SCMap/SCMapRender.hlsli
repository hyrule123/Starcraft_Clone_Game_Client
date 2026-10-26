#ifndef HLSL_SCMAP_RENDER
#define HLSL_SCMAP_RENDER
#include <Content/HLSL/SCMap/SCMapCommon.hlsli>
#include <Engine/HLSL/ForwardOpaquePass.hlsli>
#include <Engine/HLSL/Camera.hlsli>

#define REG_T_MTXM					REG_T_PER_MATERIAL_0
#define REG_T_MEGATILE_STATES		REG_T_PER_MATERIAL_1
#define REG_T_CREEP_RANDOM_TABLE	REG_T_PER_MATERIAL_2
#define REG_T_CREEP_EDGE_TEXTURE	REG_T_PER_MATERIAL_3	// Texture2DArray<uint2>, tileset\<이름>.grp
#define REG_T_CV5					REG_T_PER_MATERIAL_4
#define REG_T_VX4					REG_T_PER_MATERIAL_5
#define REG_T_VR4					REG_T_PER_MATERIAL_6


struct SCMapVSIn
{
	float3 position SEMANTIC(POSITION);
	float2 uv SEMANTIC(TEXCOORD);
	
#ifdef __HLSL
	uint instance_id : INSTANCE_ID;
#endif//__HLSL	
};

#ifdef __HLSL
struct SCMapVSOut
{
	float4 pos : SV_Position;
	float2 pixel_coord : TEXCOORD; // 픽셀 단위, UV 아님
	nointerpolation int2 map_size_megatile : MAP_SIZE_MEGATILE; //메가타일 기준 맵 사이즈
};
#endif//__HLSL

#ifdef __HLSL
// SBuffer
StructuredBuffer<MapInfoCB> g_map_info : register(REG_T_INSTANCE_BUFFER);

Buffer<uint> g_mtxm : register(REG_T_MTXM);
Buffer<uint> g_megatile_states : register(REG_T_MEGATILE_STATES); // R32_UINT, 메가타일 수
Texture2D<uint> g_creep_random_tiles : register(REG_T_CREEP_RANDOM_TABLE); // R8_UINT, 메가타일 크기
Texture2DArray<uint2> g_creep_edge : register(REG_T_CREEP_EDGE_TEXTURE); // R8G8_UINT (R: WPE 인덱스, G: 커버리지)
Buffer<uint> g_cv5 : register(REG_T_CV5);
Buffer<uint> g_vx4 : register(REG_T_VX4);
Buffer<uint> g_vr4 : register(REG_T_VR4);
#endif//__HLSL



// g_megatile_states 원소 = SCSim::MegatileState 4바이트를 그대로 담은 uint (리틀 엔디안)
//   [7:0] visible, [15:8] explored, [31:16] flags (MegatileFlags)
HLSL_CONSTEXPR uint kMegatileStateFlagsShift = 16;

// ---- Creep (OpenBW ui.h draw_tiles 기준) ----
// Creep 바닥 타일: CV5 그룹 1의 메가타일 0 ~ (kCreepTileCount - 1)
HLSL_CONSTEXPR uint kMegatileFlagHasCreep = 0x40;
	
HLSL_CONSTEXPR uint kCreepCV5Group = 1;
HLSL_CONSTEXPR uint kCreepTileBaseCount = 6; // 0~5: 기본 (선택 확률 96%)
HLSL_CONSTEXPR uint kCreepTileVariantCount = 7; // 6~12: 변형 (선택 확률 4%)
HLSL_CONSTEXPR uint kCreepTileCount = kCreepTileBaseCount + kCreepTileVariantCount;

// Creep 테두리: tileset\<이름>.grp, 캔버스 = 메가타일 1칸
// 프레임 번호 0 = 테두리 없음, 1 ~ kCreepEdgeFrameCount → GRP 프레임 0 ~ (kCreepEdgeFrameCount - 1)
HLSL_CONSTEXPR uint kCreepEdgeFrameCount = 37;
HLSL_CONSTEXPR uint kCreepEdgeCanvasSize = kMegatilePixelSize; // 32

// Creep 테두리 프레임 표 (OpenBW ui.h load_image_data의 creep_edge_frame_index)
// 인덱스 = 이웃 8칸 Creep 마스크, 값 = 테두리 프레임 번호 (0 = 없음, n = GRP 프레임 n - 1)
// 마스크 비트: 0 오른쪽아래, 1 아래, 2 왼쪽아래, 3 오른쪽, 4 왼쪽, 5 오른쪽위, 6 위, 7 왼쪽위
// C++ 쪽에서 OpenBW 규칙으로 생성한 표와 원소별로 일치하는지 static_assert로 검증함 — 직접 수정 금지
HLSL_CONSTEXPR uint kCreepEdgeFrameIndex[256] =
{
	0, 0, 7, 18, 0, 0, 12, 22, 16, 16, 20, 20, 16, 16, 34, 34, // 0x00 ~ 0x0F
	 5, 5, 13, 32, 5, 5, 13, 32, 28, 28, 35, 35, 28, 28, 35, 35, // 0x10 ~ 0x1F
	 0, 0, 7, 18, 0, 0, 12, 22, 16, 16, 20, 20, 16, 16, 34, 34, // 0x20 ~ 0x2F
	 5, 5, 13, 32, 5, 5, 13, 32, 28, 28, 35, 35, 28, 28, 35, 35, // 0x30 ~ 0x3F
	 1, 1, 8, 19, 1, 1, 14, 23, 17, 17, 21, 21, 17, 17, 36, 36, // 0x40 ~ 0x4F
	 6, 6, 15, 33, 6, 6, 15, 33, 29, 29, 37, 37, 29, 29, 37, 37, // 0x50 ~ 0x5F
	 3, 3, 10, 21, 3, 3, 25, 36, 17, 17, 21, 21, 17, 17, 36, 36, // 0x60 ~ 0x6F
	24, 24, 26, 37, 24, 24, 26, 37, 29, 29, 37, 37, 29, 29, 37, 37, // 0x70 ~ 0x7F
	 0, 0, 7, 18, 0, 0, 12, 22, 16, 16, 20, 20, 16, 16, 34, 34, // 0x80 ~ 0x8F
	 5, 5, 13, 32, 5, 5, 13, 32, 28, 28, 35, 35, 28, 28, 35, 35, // 0x90 ~ 0x9F
	 0, 0, 7, 18, 0, 0, 12, 22, 16, 16, 20, 20, 16, 16, 34, 34, // 0xA0 ~ 0xAF
	 5, 5, 13, 32, 5, 5, 13, 32, 28, 28, 35, 35, 28, 28, 35, 35, // 0xB0 ~ 0xBF
	 2, 2, 9, 30, 2, 2, 15, 33, 27, 27, 31, 31, 27, 27, 37, 37, // 0xC0 ~ 0xCF
	 6, 6, 15, 33, 6, 6, 15, 33, 29, 29, 37, 37, 29, 29, 37, 37, // 0xD0 ~ 0xDF
	 4, 4, 11, 31, 4, 4, 26, 37, 27, 27, 31, 31, 27, 27, 37, 37, // 0xE0 ~ 0xEF
	24, 24, 26, 37, 24, 24, 26, 37, 29, 29, 37, 37, 29, 29, 37, 37 // 0xF0 ~ 0xFF
};

#endif//HLSL_SCMAP_RENDER