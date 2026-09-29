#include "Content/pch.h"
#include "SCImageLibrary.h"

#include <Content/SCData/GrpFile.h>
#include <Content/SCData/MPQ/MPQFileSystem.h>

#include <Engine/Resource/GPU/Buffer/Texture2DArray.h>

#include <Engine/Core/DX11.h>

namespace engine
{
    namespace sc
    {
        namespace
        {
            constexpr DXGI_FORMAT kGrpTextureFormat = DXGI_FORMAT_R8G8_UINT;

            // GRP 하나를 디코딩해 Texture2DArray로 만들어 등록
            // pixels는 GRP 간 재사용되는 작업 버퍼 (용량 재사용 목적)
            s_ptr<Texture2DArray> BakeGrp(const MPQFileSystem& fs, const std::string& mpq_path,
                std::vector<uint8>& pixels, size_t& total_frames, size_t& total_bytes)
            {

                MPQFile file = fs.OpenFile(mpq_path);
                std::optional<std::vector<uint8>> bytes = file.ReadAll();
                if (!bytes.has_value())
                {
                    return nullptr;  // ReadAll이 이미 알림
                }

                GrpFile grp;
                if (!grp.Load(std::move(*bytes)))
                {
                    return nullptr;
                }

                const uint32 frame_count = grp.FrameCount();
                const size_t slice_pitch = grp.CanvasSizeBytes();
                const size_t row_pitch = static_cast<size_t>(grp.Width()) * GrpFile::kBytesPerPixel;

                // 투명 픽셀은 DecodeFrame이 쓰지 않으므로 매번 0으로 초기화 필수
                pixels.assign(slice_pitch * frame_count, 0);

                for (uint32 i = 0; i < frame_count; ++i)
                {
                    uint8* slice = pixels.data() + static_cast<size_t>(i) * slice_pitch;
                    if (!grp.DecodeFrame(i, slice, row_pitch))
                    {
                        return nullptr;
                    }
                }

                s_ptr<Texture2DArray> texture = EntityManager::CreateEntity<Texture2DArray>();
                const bool created = texture->CreateImmutableFromMemory(
                    uint32_2{ grp.Width(), grp.Height() }, frame_count, kGrpTextureFormat,
                    std::span<const uint8>(pixels),
                    static_cast<uint32>(row_pitch), static_cast<uint32>(slice_pitch));
                if (!created)
                {
                    return nullptr;
                }

                total_frames += frame_count;
                total_bytes += pixels.size();
                return texture;
            }
        }

        SCImageLibrary::SCImageLibrary()
        {}

        SCImageLibrary::~SCImageLibrary()
        {}

        bool SCImageLibrary::Init(const MPQFileSystem& fs)
        {
            // 1. images.dat
            const std::optional<std::vector<uint8>> dat_bytes = fs.ReadFile("arr\\images.dat");
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
            const std::optional<std::vector<uint8>> tbl_bytes = fs.ReadFile("arr\\images.tbl");
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
            std::vector<uint8> pixels;
            size_t total_frames = 0;
            size_t total_bytes = 0;

            std::vector<s_ptr<Texture2DArray>> textures;
            textures.reserve(manifest->sources.size());

            for (const GrpSource& source : manifest->sources)
            {
                s_ptr<Texture2DArray> texture = BakeGrp(fs, source.mpq_path, pixels, total_frames, total_bytes);
                if (texture == nullptr)
                {
                    LOG("SCImageLibrary: GRP 굽기 실패 {}", source.mpq_path);
                    return false;
                }
                textures.push_back(std::move(texture));
            }

            DEBUG_LOG("SCImageLibrary: GRP {}개, 프레임 {}장, 텍스처 {:.1f} MB",
                manifest->sources.size(), total_frames, static_cast<double>(total_bytes) / (1024.0 * 1024.0));

            defs_ = std::move(defs);
            manifest_ = std::move(*manifest);
			textures_ = std::move(textures);
            return true;
        }

        const ImageDef& SCImageLibrary::GetImageDef(uint32 image_type) const
        {
            return defs_.Get(image_type);  // 범위 검사는 ImageDefTable::Get의 ASSERT
        }

        s_ptr<Texture2DArray> SCImageLibrary::GetTexture(uint32 image_type) const
        {
            ASSERT(image_type < kImageTypeCount);

            const int32 source_index = manifest_.image_to_source[image_type];
            if (source_index == GrpManifest::kNoGrp)
            {
                return nullptr;
            }

            return textures_[source_index];
        }
    }
}
