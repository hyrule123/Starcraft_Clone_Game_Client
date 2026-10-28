#pragma once
#include <Engine/Core/CoreMinimal.h>

#include <span>
#include <vector>

namespace sc
{
    // 스타크래프트 PCX (8bpp 인덱스, RLE) 로더
    // 인덱스 데이터만 사용하며 파일 끝 팔레트는 무시
    class PcxFile
    {
    public:
        PcxFile() = default;

        // 원본 바이트를 보관하지 않고 로드 시 전부 디코딩하므로 span으로 받음
        bool Load(std::span<const uint8_t> bytes);

        uint32_t width() const { return width_; }
        uint32_t height() const { return height_; }

        // width × height, 행 우선, 여백 없음
        std::span<const uint8_t> pixels() const { return pixels_; }

		void Clear()
		{
			pixels_.clear();
			width_ = 0;
			height_ = 0;
		}

    private:
        std::vector<uint8_t> pixels_;
        uint32_t width_ = 0;
        uint32_t height_ = 0;
    };
}