#pragma once
#include <Engine/Core/CoreMinimal.h>

#include <Content/SCData/SCImageLibrary.h>
#include <Content/SCData/SCImageData.h>

#include <string_view>

namespace engine
{
    class Texture2DArray;
}

namespace sc
{
    class MPQFileSystem;

    // images.dat 기준으로 게임에서 쓰는 모든 GRP를 R8G8_UINT Texture2DArray로 구워
    // ResourceManager에 등록하고, ImageType → 리소스 키 / 이미지 속성 조회를 제공
    //
    // 리소스 키 = MPQ 내부 상대 경로 (예: "unit\\protoss\\zealot.grp")
    // 이름만 쓰면 폴더가 다른 동명 GRP가 충돌하므로 전체 경로 사용
    class SCImageLibrary
    {
    public:
        SCImageLibrary();
        ~SCImageLibrary();

        // 첫 실패에서 중단하고 false 반환 (원인은 하위 계층이 ERR_MSG로 알림)
        bool Init(const MPQFileSystem& fs);

        const ImageDef& GetImageDef(uint32_t image_type) const;

        uint32_t grp_count() const { return static_cast<uint32_t>(manifest_.sources.size()); }

        // GRP가 없는 이미지 타입이면 nullptr
        // 반환 포인터는 이 라이브러리가 살아있는 동안 유효 (비소유)
        std::shared_ptr<engine::Texture2DArray> GetTexture(uint32_t image_type) const;

    private:
        ImageDefTable defs_ = {};
        GrpManifest manifest_ = {};
        std::vector<std::shared_ptr<engine::Texture2DArray>> textures_ = {};  // manifest_.sources와 같은 순서
    };
}