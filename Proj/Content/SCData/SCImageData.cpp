#include "Content/pch.h"
#include "SCImageData.h"

#include <Engine/Core/Debug.h>

#include <cstring>
#include <unordered_map>
#include <span>

// ASSERT / ERR_MSG 헤더 include 필요

namespace engine
{
    namespace sc
    {
        namespace
        {
            uint16 ReadU16(const uint8* p)
            {
                uint16 value;
                std::memcpy(&value, p, sizeof(value));   // 리틀 엔디언 전제 (Windows 전용)
                return value;
            }

            // images.dat는 필드별 배열이라 한 필드를 999개 연속으로 읽어 엔트리에 흩뿌림
            template <typename StorageT, typename FieldT>
            void ReadColumn(const uint8*& cursor, std::span<ImageDef> defs, FieldT ImageDef::* field)
            {
                for (ImageDef& def : defs)
                {
                    StorageT value;
                    std::memcpy(&value, cursor, sizeof(StorageT));
                    cursor += sizeof(StorageT);
                    def.*field = static_cast<FieldT>(value);
                }
            }

            constexpr size_t kImagesDatEntrySize = 4 + 6 * 1 + 7 * 4;   // 38
            constexpr size_t kImagesDatSize = kImageTypeCount * kImagesDatEntrySize;
            static_assert(kImagesDatSize == 37962);
        }

        // ------------------------------------------------------------------
        // StringTable
        // ------------------------------------------------------------------
        bool StringTable::Load(std::vector<uint8> bytes)
        {
            if (bytes.size() < 2)
            {
                ERR_MSG("StringTable: 파일이 너무 작음 ({} bytes)", bytes.size());
                return false;
            }

            const uint32 count = ReadU16(bytes.data());
            const size_t header_size = 2 + static_cast<size_t>(count) * 2;
            if (header_size > bytes.size())
            {
                ERR_MSG("StringTable: 오프셋 테이블이 파일 범위를 벗어남 (count {}, size {})", count, bytes.size());
                return false;
            }

            // 모든 문자열이 버퍼 안에서 종료되는지 한 번에 검증 → Get()은 검사 없이 strlen 가능
            for (uint32 i = 0; i < count; ++i)
            {
                const size_t offset = ReadU16(bytes.data() + 2 + i * 2);
                if (offset >= bytes.size())
                {
                    ERR_MSG("StringTable: {}번 문자열 오프셋이 범위 밖 ({})", i + 1, offset);
                    return false;
                }

                const void* terminator = std::memchr(bytes.data() + offset, 0, bytes.size() - offset);
                if (terminator == nullptr)
                {
                    ERR_MSG("StringTable: {}번 문자열이 종료되지 않음", i + 1);
                    return false;
                }
            }

            bytes_ = std::move(bytes);
            count_ = count;
            return true;
        }

        std::string_view StringTable::Get(uint32 index) const
        {
            if (index == 0 || index > count_)
            {
                return {};
            }

            const size_t offset = ReadU16(bytes_.data() + 2 + (index - 1) * 2);
            const char* str = reinterpret_cast<const char*>(bytes_.data() + offset);
            return std::string_view(str, std::strlen(str));
        }

        // ------------------------------------------------------------------
        // ImageDefTable
        // ------------------------------------------------------------------
        bool ImageDefTable::Load(const std::vector<uint8>& bytes)
        {
            if (bytes.size() != kImagesDatSize)
            {
                ERR_MSG("images.dat 크기 불일치: {} bytes (expected {}), 버전이 다를 수 있음", bytes.size(), kImagesDatSize);
                return false;
            }

            std::vector<ImageDef> defs{};
			defs.resize(kImageTypeCount);
            const uint8* cursor = bytes.data();

            // 파일에 저장된 순서 그대로
            ReadColumn<uint32>(cursor, defs, &ImageDef::grp_index);
            ReadColumn<uint8>(cursor, defs, &ImageDef::has_directional_frames);
            ReadColumn<uint8>(cursor, defs, &ImageDef::is_clickable);
            ReadColumn<uint8>(cursor, defs, &ImageDef::has_iscript_animations);
            ReadColumn<uint8>(cursor, defs, &ImageDef::draw_if_cloaked);
            ReadColumn<uint8>(cursor, defs, &ImageDef::draw_function);
            ReadColumn<uint8>(cursor, defs, &ImageDef::remapping);
            ReadColumn<uint32>(cursor, defs, &ImageDef::iscript_id);
            ReadColumn<uint32>(cursor, defs, &ImageDef::shield_overlay_index);
            ReadColumn<uint32>(cursor, defs, &ImageDef::attack_overlay_index);
            ReadColumn<uint32>(cursor, defs, &ImageDef::damage_overlay_index);
            ReadColumn<uint32>(cursor, defs, &ImageDef::special_overlay_index);
            ReadColumn<uint32>(cursor, defs, &ImageDef::landing_dust_overlay_index);
            ReadColumn<uint32>(cursor, defs, &ImageDef::lift_off_overlay_index);

            ASSERT(cursor == bytes.data() + bytes.size());

            defs_ = std::move(defs);
            return true;
        }

        const ImageDef& ImageDefTable::Get(uint32 imageType) const
        {
            // 호출자가 넘기는 값은 시뮬레이션/엔진 내부 데이터 → 프로그래머 실수는 ASSERT
            ASSERT(imageType < defs_.size());
            return defs_[imageType];
        }

        // ------------------------------------------------------------------
        // GrpManifest
        // ------------------------------------------------------------------
        std::optional<GrpManifest> BuildGrpManifest(const ImageDefTable& defs, const StringTable& images_table)
        {
            auto manifest = GrpManifest{};
            manifest.image_to_source.resize(kImageTypeCount, GrpManifest::kNoGrp);

            std::unordered_map<uint32, int32> table_index_to_source;

            for (uint32 image_type = 0; image_type < kImageTypeCount; ++image_type)
            {
                const uint32 table_idx = defs.Get(image_type).grp_index;
                if (table_idx == 0)
                {
                    continue;
                }

                auto found = table_index_to_source.find(table_idx);
                if (found != table_index_to_source.end())
                {
                    manifest.image_to_source[image_type] = found->second;
                    continue;
                }

                const std::string_view relative_path = images_table.Get(table_idx);
                if (relative_path.empty())
                {
                    ERR_MSG("images.dat {}번 이미지의 GRP 인덱스 {}가 images.tbl에 없음", image_type, table_idx);
                    return std::nullopt;
                }

                GrpSource source;
                source.tbl_index = table_idx;
                source.mpq_path.reserve(5 + relative_path.size());
                source.mpq_path = "unit\\";
                source.mpq_path += relative_path;

                const int32 source_idx = static_cast<int32>(manifest.sources.size());
                manifest.sources.push_back(std::move(source));
                table_index_to_source.emplace(table_idx, source_idx);
                manifest.image_to_source[image_type] = source_idx;
            }

            return manifest;
        }
    }
}
