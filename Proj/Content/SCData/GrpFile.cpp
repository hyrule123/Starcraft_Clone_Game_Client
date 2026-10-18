#include "Content/pch.h"
#include "GrpFile.h"

#include <Engine/Core/Debug.h>

#include <algorithm>
#include <cstring>


namespace sc
{
    namespace
    {
        constexpr size_t kHeaderSize = 6;
        constexpr size_t kFrameEntrySize = 8;

        constexpr uint8_t kSkipFlag = 0x80;    // 투명 건너뛰기, 하위 7비트 = 길이
        constexpr uint8_t kRepeatFlag = 0x40;  // 같은 색 반복, 하위 6비트 = 길이, 다음 1바이트 = 색
        // 둘 다 아님: 이어지는 N바이트를 그대로 사용

        template <typename T>
        T ReadLE(const uint8_t* p)
        {
            T value;
            std::memcpy(&value, p, sizeof(T));  // 리틀 엔디언 전제 (Windows 전용)
            return value;
        }

        void WritePixel(uint8_t* out, uint8_t value)
        {
            out[0] = value;  // R: 인덱스
            out[1] = 1;      // G: 커버리지
        }
    }

    bool GrpFile::Load(std::vector<uint8_t> bytes)
    {
        if (bytes.size() < kHeaderSize)
        {
            ERR_MSG("GRP: 헤더보다 작은 파일 ({} bytes)", bytes.size());
            return false;
        }

        const uint16_t frame_count = ReadLE<uint16_t>(bytes.data() + 0);
        const uint16_t canvas_width = ReadLE<uint16_t>(bytes.data() + 2);
        const uint16_t canvas_height = ReadLE<uint16_t>(bytes.data() + 4);

        if (frame_count == 0 || canvas_width == 0 || canvas_height == 0)
        {
            ERR_MSG("GRP: 비정상 헤더 (frames {}, {}x{})", frame_count, canvas_width, canvas_height);
            return false;
        }

        const size_t table_end = kHeaderSize + static_cast<size_t>(frame_count) * kFrameEntrySize;
        if (table_end > bytes.size())
        {
            ERR_MSG("GRP: 프레임 테이블이 파일 범위를 벗어남 (frames {}, size {})", frame_count, bytes.size());
            return false;
        }

        std::vector<GrpFrameInfo> frames(frame_count);
        for (uint32_t i = 0; i < frame_count; ++i)
        {
            const uint8_t* entry = bytes.data() + kHeaderSize + i * kFrameEntrySize;

            GrpFrameInfo& info = frames[i];
            info.offset_x = entry[0];
            info.offset_y = entry[1];
            info.width = entry[2];
            info.height = entry[3];
            info.data_offset = ReadLE<uint32_t>(entry + 4);

            // 캔버스 밖으로 쓰는 프레임은 호출자 버퍼(캔버스 크기)를 넘어서므로 거부
            if (info.offset_x + info.width > canvas_width || info.offset_y + info.height > canvas_height)
            {
                ERR_MSG("GRP: {}번 프레임이 캔버스를 벗어남 ({},{} {}x{} / canvas {}x{})",
                    i, info.offset_x, info.offset_y, info.width, info.height, canvas_width, canvas_height);
                return false;
            }

            if (info.width == 0 || info.height == 0)
            {
                continue;  // 빈 프레임은 데이터 검증 불필요
            }

            // 행 오프셋 테이블: height개의 uint16_t, data_offset 기준 상대값
            const size_t row_table_end = static_cast<size_t>(info.data_offset) + static_cast<size_t>(info.height) * 2;
            if (row_table_end > bytes.size())
            {
                ERR_MSG("GRP: {}번 프레임의 행 오프셋 테이블이 범위 밖", i);
                return false;
            }

            for (uint32_t row = 0; row < info.height; ++row)
            {
                const size_t row_start = info.data_offset + ReadLE<uint16_t>(bytes.data() + info.data_offset + row * 2);
                if (row_start >= bytes.size())
                {
                    ERR_MSG("GRP: {}번 프레임 {}행의 데이터 위치가 범위 밖", i, row);
                    return false;
                }
            }
        }

        bytes_ = std::move(bytes);
        frames_ = std::move(frames);
        width_ = canvas_width;
        height_ = canvas_height;
        return true;
    }

    const GrpFrameInfo& GrpFile::Frame(uint32_t frame_index) const
    {
        ASSERT(frame_index < frames_.size());
        return frames_[frame_index];
    }

    bool GrpFile::DecodeFrame(uint32_t frame_index, uint8_t* dest, size_t dest_pitch) const
    {
        // 호출 인자는 엔진 내부 값 → ASSERT
        ASSERT(frame_index < frames_.size());
        ASSERT(dest != nullptr);
        ASSERT(dest_pitch >= static_cast<size_t>(width_) * kBytesPerPixel);

        const GrpFrameInfo& info = frames_[frame_index];
        const uint8_t* const data_begin = bytes_.data();
        const uint8_t* const data_end = data_begin + bytes_.size();

        for (uint32_t row = 0; row < info.height; ++row)
        {
            const size_t row_start = info.data_offset + ReadLE<uint16_t>(data_begin + info.data_offset + row * 2);
            const uint8_t* src = data_begin + row_start;  // Load에서 범위 검증됨

            uint8_t* out = dest
                + static_cast<size_t>(info.offset_y + row) * dest_pitch
                + static_cast<size_t>(info.offset_x) * kBytesPerPixel;

            uint32_t x = 0;
            while (x < info.width)
            {
                if (src >= data_end)
                {
                    ERR_MSG("GRP: {}번 프레임 {}행 RLE가 파일 끝을 넘음", frame_index, row);
                    return false;
                }

                const uint8_t control = *src++;
                const uint32_t remaining = info.width - x;

                if (control & kSkipFlag)
                {
                    const uint32_t run = control & 0x7F;
                    if (run == 0)
                    {
                        ERR_MSG("GRP: {}번 프레임 {}행에 길이 0 런", frame_index, row);
                        return false;
                    }

                    // 투명: 쓰지 않고 넘어감 (호출자 버퍼가 0으로 초기화되어 있음)
                    const uint32_t count = std::min(run, remaining);
                    out += count * kBytesPerPixel;
                    x += count;
                }
                else if (control & kRepeatFlag)
                {
                    const uint32_t run = control & 0x3F;
                    if (run == 0 || src >= data_end)
                    {
                        ERR_MSG("GRP: {}번 프레임 {}행 반복 런이 비정상", frame_index, row);
                        return false;
                    }

                    const uint8_t value = *src++;
                    const uint32_t count = std::min(run, remaining);
                    for (uint32_t i = 0; i < count; ++i)
                    {
                        WritePixel(out, value);
                        out += kBytesPerPixel;
                    }
                    x += count;
                }
                else
                {
                    const uint32_t run = control;
                    if (run == 0)
                    {
                        ERR_MSG("GRP: {}번 프레임 {}행에 길이 0 런", frame_index, row);
                        return false;
                    }

                    const uint32_t count = std::min(run, remaining);
                    if (static_cast<size_t>(data_end - src) < count)
                    {
                        ERR_MSG("GRP: {}번 프레임 {}행 리터럴 런이 파일 끝을 넘음", frame_index, row);
                        return false;
                    }

                    for (uint32_t i = 0; i < count; ++i)
                    {
                        WritePixel(out, src[i]);
                        out += kBytesPerPixel;
                    }
                    src += count;
                    x += count;
                }
            }
        }

        return true;
    }
}
