#pragma once
#include <Content/SCData/MPQ/MPQFileSystem.h>

#include <Engine/Core/CoreMinimal.h>

#include <array>
#include <span>
#include <string_view>
#include <vector>

namespace sc
{
    // 효과 테이블 번호. RT1에 기록하는 "효과 종류"와 같은 값을 사용
    // light 계열 순서는 OpenBW light_names와 동일
    // (OpenBW draw_alpha(4, ...)의 4 = light 배열 기준 trans50 → 여기서는 kTrans50 = 5)
    enum class PcxLUT : uint8_t
    {
        Dark = 0,
        Ofire,
        Gfire,
        Bfire,
        Bexpl,
        Trans50,
        Red,
        Green,
        kCount
    };

    // 타일셋 하나의 dark + light 7종 LUT를 하나의 연속 버퍼로 이어 붙여 보관 (CPU 측)
    // 모든 테이블은 너비 256, 조회는 data[range.offset + row * 256 + dst_index]
    class SCTileSetPcxLUT
    {
    public:
		using ElemType = uint8_t;

        static constexpr uint32_t kTableWidth = 256;
        static constexpr uint32_t kTableCount = static_cast<uint32_t>(PcxLUT::kCount);

        struct TableRange
        {
            uint32_t offset = 0;  // data() 내 시작 위치 (바이트)
            uint32_t rows = 0;    // 행 수 (light 계열은 소스 값 - 1이 행 번호)
        };

        SCTileSetPcxLUT();
        ~SCTileSetPcxLUT();

        // tileset_name: "badlands", "platform", ... (맵 로더와 같은 이름 목록 사용)
        // 첫 실패에서 중단, 전체 성공 시에만 내부 상태 반영
        bool Init(const MPQFileSystem& fs, std::string_view tileset_name);

        std::span<const ElemType> data() const { return data_; }
		std::span<const TableRange> ranges() const { return ranges_; }
        const TableRange& range(PcxLUT table) const;

    private:
        std::vector<ElemType> data_ = {};
        std::array<TableRange, kTableCount> ranges_ = {};
    };
}