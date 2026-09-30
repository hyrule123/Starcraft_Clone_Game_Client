#pragma once
#include <Engine/Core/CoreMinimal.h>

#include <array>
#include <string>
#include <string_view>
#include <vector>

namespace sc
{
    constexpr size_t kImageTypeCount = 999;

    // ------------------------------------------------------------------
    // images.tbl 등 스타크래프트 문자열 테이블
    // [uint16 count][uint16 offset × count][null-terminated strings...]
    // 인덱스는 1부터 시작, 0은 "없음"
    // ------------------------------------------------------------------
    class StringTable
    {
    public:
        StringTable() = default;

        // 원본 바이트를 소유권째 받아 보관. 모든 문자열의 오프셋/종료를 로드 시 검증
        bool Load(std::vector<uint8> bytes);

        // 반환된 view는 이 테이블이 살아있는 동안만 유효
        // index가 0이거나 범위 밖이면 빈 view
        std::string_view Get(uint32 index) const;

        uint32 Count() const { return count_; }

    private:
        std::vector<uint8> bytes_;
        uint32 count_ = 0;
    };

    // ------------------------------------------------------------------
    // images.dat 엔트리 (파일은 필드별 배열(SoA), 여기서는 엔트리별(AoS)로 재배치)
    // 모든 *Index 필드는 images.tbl 인덱스(1-based, 0 = 없음)
    // ------------------------------------------------------------------
    struct ImageDef
    {
        uint32 grp_index = 0;
        bool   has_directional_frames = false;
        bool   is_clickable = false;
        bool   has_iscript_animations = false;
        bool   draw_if_cloaked = false;
        uint8  draw_function = 0;   // OpenBW: modifier
        uint8  remapping = 0;      // OpenBW: color_shift
        uint32 iscript_id = 0;
        uint32 shield_overlay_index = 0;
        uint32 attack_overlay_index = 0;
        uint32 damage_overlay_index = 0;
        uint32 special_overlay_index = 0;
        uint32 landing_dust_overlay_index = 0;
        uint32 lift_off_overlay_index = 0;
    };

    class ImageDefTable
    {
    public:
        ImageDefTable() = default;

        // 크기가 정확히 999 × 38 바이트가 아니면 실패
        bool Load(const std::vector<uint8>& bytes);

        const ImageDef& Get(uint32 imageType) const;

    private:
        // size: kImageTypeCount
        std::vector<ImageDef> defs_ = {};
    };

    // ------------------------------------------------------------------
    // 굽기 대상 GRP 목록
    // 중복 제거는 경로 문자열이 아니라 images.tbl 인덱스 기준 (OpenBW와 동일)
    // ------------------------------------------------------------------
    struct GrpSource
    {
        uint32      tbl_index = 0;
        std::string mpq_path;   // "unit\\protoss\\zealot.grp"
    };

    struct GrpManifest
    {
        static constexpr int32 kNoGrp = -1;

        std::vector<GrpSource> sources;
        // ImageType → sources 인덱스, GRP가 없으면 kNoGrp
        std::vector<int32> image_to_source;
    };

    [[nodiscard]] std::optional<GrpManifest> BuildGrpManifest(const ImageDefTable& defs, const StringTable& images_table);
}
