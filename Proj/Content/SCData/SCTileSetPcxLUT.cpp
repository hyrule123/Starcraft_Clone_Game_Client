#include "Content/pch.h"
#include "SCTileSetPcxLUT.h"

#include <Content/SCData/PCXFile.h>

#include <Content/HLSL/SCImage/SCImageCommon.hlsli>

#include <Engine/Core/Debug.h>

#include <format>

namespace sc
{
	static_assert(kPcxLutTableWidth == SCTileSetPcxLUT::kTableWidth, 
        "SCImageCommon.hlsli의 kPcxLutTableWidth와 SCTileSetPcxLUT::kTableWidth가 일치하지 않음");

	// HLSL - C++ 상수 검증
    static_assert(kPcxLutDarkIdx == static_cast<uint32_t>(PcxLUT::Dark));
    static_assert(kPcxLutOfireIdx == static_cast<uint32_t>(PcxLUT::Ofire));
    static_assert(kPcxLutGfireIdx == static_cast<uint32_t>(PcxLUT::Gfire));
    static_assert(kPcxLutBfireIdx == static_cast<uint32_t>(PcxLUT::Bfire));
    static_assert(kPcxLutBexplIdx == static_cast<uint32_t>(PcxLUT::Bexpl));
    static_assert(kPcxLutTrans50Idx== static_cast<uint32_t>(PcxLUT::Trans50));
    static_assert(kPcxLutRedIdx == static_cast<uint32_t>(PcxLUT::Red));
    static_assert(kPcxLutGreenIdx == static_cast<uint32_t>(PcxLUT::Green));
    static_assert(kPcxLutCount == static_cast<uint32_t>(PcxLUT::kCount));

    namespace
    {
        // PcxLUT 순서와 반드시 일치
        constexpr std::array<std::string_view, SCTileSetPcxLUT::kTableCount> kTableFileNames =
        {
            "dark",
            "ofire",
            "gfire",
            "bfire",
            "bexpl",
            "trans50",
            "red",
            "green",
        };

        constexpr uint32_t kDarkRows = 32;
        constexpr uint32_t kDarkIdentityRow = 31;  // OpenBW가 로드 후 항등 테이블로 덮어쓰는 행
    }

    SCTileSetPcxLUT::SCTileSetPcxLUT()
    {}

    SCTileSetPcxLUT::~SCTileSetPcxLUT()
    {}

    bool SCTileSetPcxLUT::Init(const MPQFileSystem& fs, std::string_view tileset_name)
    {
        std::vector<uint8_t> data;
        std::array<TableRange, kTableCount> ranges{};

        for (uint32_t i = 0; i < kTableCount; ++i)
        {
            const std::string path = std::format("tileset\\{}\\{}.pcx", tileset_name, kTableFileNames[i]);

			MPQFile file = fs.OpenFile(path);
            const std::optional<std::vector<uint8_t>> bytes = fs.ReadFile(path);
            if (!bytes.has_value())
            {
                return false;  // ReadFile이 이미 알림
            }

            PcxFile pcx;
            if (!pcx.Load(*bytes))
            {
                LOG("SCTileSetPcxLUT: Failed to load PCX {}", path);
                return false;
            }

            if (pcx.width() != kTableWidth)
            {
                ERR_MSG("SCTileSetPcxLUT: Table width is not 256 {} ({}x{})", path, pcx.width(), pcx.height());
                return false;
            }

            const bool is_dark = (i == static_cast<uint32_t>(PcxLUT::Dark));
            if (is_dark && pcx.height() != kDarkRows)
            {
                ERR_MSG("SCTileSetPcxLUT: dark.pcx는 256x32여야 함 {} ({}x{})", path, pcx.width(), pcx.height());
                return false;
            }

            const uint32_t offset = static_cast<uint32_t>(data.size());
            ranges[i].offset = offset;
            ranges[i].rows = pcx.height();

            const std::span<const uint8_t> pixels = pcx.pixels();
            data.insert(data.end(), pixels.begin(), pixels.end());

            // OpenBW와 동일: dark 31번 행을 항등 테이블(x → x)로 덮어씀
            if (is_dark)
            {
                uint8_t* row = data.data() + offset + kDarkIdentityRow * kTableWidth;
                for (uint32_t x = 0; x < kTableWidth; ++x)
                {
                    row[x] = static_cast<uint8_t>(x);
                }
            }

            //ASSERT(range(PcxLUT::Dark).offset == kPcxLutDarkOffset);
        }

        DEBUG_LOG("SCTileSetPcxLUT [{}]: dark {} / ofire {} / gfire {} / bfire {} / bexpl {} / trans50 {} / red {} / green {} rows, total {} bytes",
            tileset_name,
            ranges[0].rows, ranges[1].rows, ranges[2].rows, ranges[3].rows,
            ranges[4].rows, ranges[5].rows, ranges[6].rows, ranges[7].rows,
            data.size());

        data_ = std::move(data);
        ranges_ = ranges;
        return true;
    }

    const SCTileSetPcxLUT::TableRange& SCTileSetPcxLUT::range(PcxLUT table) const
    {
        const uint32_t index = static_cast<uint32_t>(table);
        ASSERT(index < kTableCount);
        return ranges_[index];
    }
}