#pragma once
#include <Engine/Core/CoreMinimal.h>

#include <vector>

namespace engine
{
    namespace sc
    {
        // GRP 프레임 테이블 엔트리 (파일 레이아웃과 동일 순서)
        struct GrpFrameInfo
        {
            uint8 offset_x = 0;      // 캔버스 내 x 위치
            uint8 offset_y = 0;      // 캔버스 내 y 위치
            uint8 width = 0;         // 잘린 프레임 너비
            uint8 height = 0;        // 잘린 프레임 높이
            uint32 data_offset = 0;  // 파일 시작 기준, 행 오프셋 테이블 위치
        };

        // GRP 해석만 담당. 목적지(텍스처 배열 슬라이스 등)의 배치는 모름
        class GrpFile
        {
        public:
            // 출력 픽셀 = R8G8 (R: WPE 인덱스 또는 효과 값, G: 커버리지 1)
            static constexpr size_t kBytesPerPixel = 2;

            GrpFile() = default;

            // 헤더, 프레임 테이블, 행 오프셋 테이블까지 검증. RLE는 DecodeFrame에서 검증
            bool Load(std::vector<uint8> bytes);

            uint32 FrameCount() const { return static_cast<uint32>(frames_.size()); }
            uint16 Width() const { return width_; }
            uint16 Height() const { return height_; }
            const GrpFrameInfo& Frame(uint32 frame_index) const;

            // 캔버스 한 장(슬라이스 하나)의 바이트 수
            size_t CanvasSizeBytes() const
            {
                return static_cast<size_t>(width_) * height_ * kBytesPerPixel;
            }

            // dest: 캔버스(width × height) 좌상단. 호출자가 미리 0으로 초기화해야 함
            //       (투명 픽셀은 쓰지 않고 건너뛰므로 초기값 0이 곧 커버리지 0)
            // dest_pitch: 한 행의 바이트 수 (최소 width × kBytesPerPixel)
            bool DecodeFrame(uint32 frame_index, uint8* dest, size_t dest_pitch) const;

        private:
            std::vector<uint8> bytes_;
            std::vector<GrpFrameInfo> frames_;
            uint16 width_ = 0;
            uint16 height_ = 0;
        };
    }
}
