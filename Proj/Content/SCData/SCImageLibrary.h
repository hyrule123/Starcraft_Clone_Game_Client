#pragma once
#include <Engine/Core/CoreMinimal.h>

#include <Content/SCData/SCImageLibrary.h>
#include <Content/SCData/SCImageData.h>

#include <string_view>

namespace engine
{
    class Texture2DArray;
    class StructuredBuffer;
}

namespace sc
{
    class MPQFileSystem;

    // BakeGrp 결과. 실패 시 둘 다 nullptr
    struct BakedGrp
    {
        std::shared_ptr<engine::Texture2DArray> texture = {};      // GRP 하나를 디코딩해 Texture2DArray로 만들어 등록
        std::shared_ptr<engine::StructuredBuffer> frame_rects = {};	// 캔버스 내 프레임의 위치 정보(캔버스 좌상단으로부터 오프셋, 크기)
    };

    // images.dat 기준으로 게임에서 쓰는 모든 GRP를 R8G8_UINT Texture2DArray로 구워
    // ResourceManager에 등록하고, ImageType → 리소스 키 / 이미지 속성 조회를 제공
    //
    // 리소스 키 = MPQ 내부 상대 경로 (예: "unit\\protoss\\zealot.grp")
    // 이름만 쓰면 폴더가 다른 동명 GRP가 충돌하므로 전체 경로 사용

    // 참고: '캔버스'와 '프레임'
    // SC는 데이터 절약을 위해 '캔버스'와 '프레임'을 구분 중
    // '캔버스' = 텍스처 전체 크기
	// '프레임' = 캔버스 내에서 실제로 쓰는 영역( = 픽셀이 존재하는 영역)
    class SCImageLibrary
    {
    public:
        SCImageLibrary();
        ~SCImageLibrary();

        // 첫 실패에서 중단하고 false 반환 (원인은 하위 계층이 ERR_MSG로 알림)
        bool Init(const MPQFileSystem& fs);

        const ImageDef& GetImageDef(int32_t image_type) const;

        uint32_t grp_count() const { return static_cast<int32_t>(manifest_.sources.size()); }

        // GRP가 없는 이미지 타입이면 내부에 nullptr이 들어있음.
		BakedGrp GetGrp(int32_t image_type) const;

    private:
        ImageDefTable defs_ = {};
        GrpManifest manifest_ = {};
		std::vector<BakedGrp> grp_resources_ = {};  // manifest_.sources와 같은 순서
    };
}