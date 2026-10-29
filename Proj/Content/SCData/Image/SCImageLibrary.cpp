#include "Content/pch.h"
#include "SCImageLibrary.h"

#include <Content/HLSL/SCImage/SCImageCommon.hlsli>

#include <Content/SCData/Format/GrpFile.h>
#include <Content/SCData/Format/MPQ/MPQFileSystem.h>

#include <Engine/Resource/GPU/Buffer/Texture2DArray.h>
#include <Engine/Resource/GPU/Buffer/StructuredBuffer.h>

#include <Engine/Core/DX11.h>

namespace sc
{
    namespace
    {
        constexpr DXGI_FORMAT kGrpTextureFormat = DXGI_FORMAT_R8G8_UINT;
    }
    
    BakedGrp SCImageLibrary::BakeGrp(std::vector<uint8_t> grp_bytes, std::vector<uint8_t>& pixels)
    {
        using namespace engine;

        GrpFile grp;
        if (!grp.Load(std::move(grp_bytes)))
        {
            return {};
        }

        const uint32_t frame_count = grp.FrameCount();
        const size_t slice_pitch = grp.CanvasSizeBytes();
        const size_t row_pitch = static_cast<size_t>(grp.Width()) * GrpFile::kBytesPerPixel;

        // 투명 픽셀은 DecodeFrame이 쓰지 않으므로 매번 0으로 초기화 필수
        pixels.assign(slice_pitch * frame_count, 0);

        for (uint32_t i = 0; i < frame_count; ++i)
        {
            uint8_t* slice = pixels.data() + static_cast<size_t>(i) * slice_pitch;
            if (!grp.DecodeFrame(i, slice, row_pitch))
            {
                return {};
            }
        }

        std::shared_ptr<Texture2DArray> texture = EntityManager::CreateEntity<Texture2DArray>();
        const bool created = texture->CreateImmutableFromMemory(
            uint32_2{ grp.Width(), grp.Height() }, frame_count, kGrpTextureFormat,
            std::span<const uint8_t>(pixels),
            static_cast<uint32_t>(row_pitch), static_cast<uint32_t>(slice_pitch));
        if (!created)
        {
            return {};
        }

        std::vector<GrpFrameRect> frame_rects;
        frame_rects.reserve(frame_count);
        for (uint32_t i = 0; i < frame_count; ++i)
        {
            const GrpFrameInfo& f = grp.Frame(i);
            frame_rects.push_back(
                {
                    .offset = uint32_2{ f.offset_x, f.offset_y },
                    .size = uint32_2{ f.width, f.height }
                }
            );
        }

        std::shared_ptr<engine::StructuredBuffer> rects_buf = EntityManager::CreateEntity<engine::StructuredBuffer>();
        const bool rects_created = rects_buf->CreateImmutableBuffer(std::span<const GrpFrameRect>(frame_rects));
        if (!rects_created) { return {}; }

        return { .texture = std::move(texture), .frame_rects_gpu_buf = std::move(rects_buf), .frame_rects = std::move(frame_rects) };
    }

    SCImageLibrary::SCImageLibrary()
    {}

    SCImageLibrary::~SCImageLibrary()
    {}

    bool SCImageLibrary::Init(const MPQFileSystem& fs)
    {
        using namespace engine;

        // 1. images.dat
        const std::optional<std::vector<uint8_t>> dat_bytes = fs.ReadFile("arr\\images.dat");
        if (!dat_bytes.has_value())
        {
            return false;
        }

        ImageDefTable defs;
        if (defs.Load(dat_bytes.value()) == false)
        {
            return false;
        }

        // 2. images.tbl (매니페스트 생성에만 쓰고 버림, 경로는 매니페스트에 복사됨)
        const std::optional<std::vector<uint8_t>> tbl_bytes = fs.ReadFile("arr\\images.tbl");
        if (!tbl_bytes.has_value())
        {
            return false;
        }

        StringTable images_table;
        if (!images_table.Load(*tbl_bytes))
        {
            return false;
        }

        // 3. 굽기 대상 목록
		std::optional<GrpManifest> manifest = BuildGrpManifest(defs, images_table);
        if (!manifest.has_value())
        {
            return false;
        }

        // 4. GRP별 굽기
        std::vector<uint8_t> pixels;
        size_t total_frames = 0;
        size_t total_bytes = 0;

		std::vector<BakedGrp> grp_resources;
        grp_resources.reserve(manifest->sources.size());

        for (const GrpSource& source : manifest->sources)
        {
            std::optional<std::vector<uint8_t>> grp_bytes = fs.ReadFile(source.mpq_path);
            if (!grp_bytes.has_value())
            {
                LOG("SCImageLibrary: GRP 읽기 실패 {}", source.mpq_path);
                return false;
            }

            BakedGrp baked = BakeGrp(std::move(*grp_bytes), pixels);
            if (baked.texture == nullptr || baked.frame_rects_gpu_buf == nullptr)
            {
                LOG("SCImageLibrary: GRP 굽기 실패 {}", source.mpq_path);
                return false;
            }
            
			total_frames += baked.texture->GetFrameCount();
			total_bytes += 
                static_cast<size_t>(GetDXGIFormatByteStride(baked.texture->GetFormat())) * 
                static_cast<size_t>(baked.texture->GetWidth()) * 
                static_cast<size_t>(baked.texture->GetHeight()) * 
                static_cast<size_t>(baked.texture->GetFrameCount());

			grp_resources.push_back(std::move(baked));
        }

        DEBUG_LOG("SCImageLibrary: GRP {}개, 프레임 {}장, 텍스처 {:.1f} MB",
            manifest->sources.size(), total_frames, static_cast<double>(total_bytes) / (1024.0 * 1024.0));

        defs_ = std::move(defs);
        manifest_ = std::move(*manifest);
		grp_resources_ = std::move(grp_resources);
        return true;
    }

    const ImageDef& SCImageLibrary::GetImageDef(int32_t image_type) const
    {
        return defs_.Get(image_type);  // 범위 검사는 ImageDefTable::Get의 ASSERT
    }

    const BakedGrp& SCImageLibrary::GetGrp(int32_t image_type) const
    {
        ASSERT(0 <= image_type && static_cast<size_t>(image_type) < kImageTypeCount);
        const int32_t source_index = manifest_.image_to_source[image_type];
        if (source_index == GrpManifest::kNoGrp) 
        { 
            const static BakedGrp empty = {}; 
            return empty;
        }
        return grp_resources_[source_index];
    }
}
