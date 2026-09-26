#include "Content/pch.h"
#include "PcxFile.h"

#include <Engine/Core/Debug.h>

#include <cstring>
#include <algorithm>

namespace engine
{
    namespace sc
    {
        namespace
        {
            constexpr size_t kHeaderSize = 128;

            // 헤더 필드 오프셋
            constexpr size_t kOffsetManufacturer = 0;   // uint8, 0x0A
            constexpr size_t kOffsetEncoding = 2;       // uint8, 1 = RLE
            constexpr size_t kOffsetBitsPerPixel = 3;   // uint8
            constexpr size_t kOffsetXMin = 4;           // uint16
            constexpr size_t kOffsetYMin = 6;           // uint16
            constexpr size_t kOffsetXMax = 8;           // uint16
            constexpr size_t kOffsetYMax = 10;          // uint16
            constexpr size_t kOffsetPlanes = 65;        // uint8
            constexpr size_t kOffsetBytesPerLine = 66;  // uint16

            constexpr uint8 kManufacturer = 0x0A;
            constexpr uint8 kRunFlag = 0xC0;   // 상위 두 비트가 11이면 반복 런
            constexpr uint8 kRunMask = 0x3F;

            uint16 ReadU16(const uint8* p)
            {
                uint16 value;
                std::memcpy(&value, p, sizeof(value));  // 리틀 엔디언 전제 (Windows 전용)
                return value;
            }
        }

        bool PcxFile::Load(std::span<const uint8> bytes)
        {
            if (bytes.size() < kHeaderSize)
            {
                ERR_MSG("PCX: 헤더보다 작은 파일 ({} bytes)", bytes.size());
                return false;
            }

            const uint8* header = bytes.data();

            if (header[kOffsetManufacturer] != kManufacturer)
            {
                ERR_MSG("PCX: 식별자 불일치 ({:#x})", header[kOffsetManufacturer]);
                return false;
            }

            if (header[kOffsetEncoding] != 1 || header[kOffsetBitsPerPixel] != 8 || header[kOffsetPlanes] != 1)
            {
                ERR_MSG("PCX: 8bpp 단일 플레인 RLE만 지원 (encoding {}, bpp {}, planes {})",
                    header[kOffsetEncoding], header[kOffsetBitsPerPixel], header[kOffsetPlanes]);
                return false;
            }

            const uint16 x_min = ReadU16(header + kOffsetXMin);
            const uint16 y_min = ReadU16(header + kOffsetYMin);
            const uint16 x_max = ReadU16(header + kOffsetXMax);
            const uint16 y_max = ReadU16(header + kOffsetYMax);

            if (x_min != 0 || y_min != 0 || x_max < x_min || y_max < y_min)
            {
                ERR_MSG("PCX: 비정상 좌표 범위 ({},{})-({},{})", x_min, y_min, x_max, y_max);
                return false;
            }

            const uint32 width = static_cast<uint32>(x_max) + 1;
            const uint32 height = static_cast<uint32>(y_max) + 1;

            // 행 끝 여백이 있는 파일은 지원하지 않음 (OpenBW와 동일, 스타크래프트 데이터에는 없음)
            const uint16 bytes_per_line = ReadU16(header + kOffsetBytesPerLine);
            if (bytes_per_line != width)
            {
                ERR_MSG("PCX: 행 여백 미지원 (bytes_per_line {}, width {})", bytes_per_line, width);
                return false;
            }

            std::vector<uint8> pixels(static_cast<size_t>(width) * height);

            // RLE는 행 경계와 무관하게 연속 스트림으로 디코딩 (여백이 없으므로 동일)
            const uint8* src = bytes.data() + kHeaderSize;
            const uint8* const src_end = bytes.data() + bytes.size();
            uint8* dst = pixels.data();
            uint8* const dst_end = pixels.data() + pixels.size();

            while (dst < dst_end)
            {
                if (src >= src_end)
                {
                    ERR_MSG("PCX: 픽셀 데이터가 파일 끝을 넘음");
                    return false;
                }

                const uint8 control = *src++;
                if ((control & kRunFlag) == kRunFlag)
                {
                    if (src >= src_end)
                    {
                        ERR_MSG("PCX: 반복 런 값이 파일 끝을 넘음");
                        return false;
                    }

                    const uint8 value = *src++;
                    const size_t remaining = static_cast<size_t>(dst_end - dst);
                    const size_t count = std::min<size_t>(control & kRunMask, remaining);
                    std::memset(dst, value, count);
                    dst += count;
                }
                else
                {
                    *dst++ = control;  // 상위 두 비트가 11이 아니면 그 자체가 픽셀 값
                }
            }

            pixels_ = std::move(pixels);
            width_ = width;
            height_ = height;
            return true;
        }
    }
}