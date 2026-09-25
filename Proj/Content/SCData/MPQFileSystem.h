#pragma once

#include <Engine/Core/CoreMinimal.h>

namespace engine
{
    // 우선순위 순서로 MPQ를 보관하고, 먼저 발견된 파일을 사용 (OpenBW와 동일 규칙)
    class MPQArchive;

    class MPQFileSystem
    {
    public:
        // 추가 순서 = 검색 우선순위. required가 false면 파일이 없어도 성공
        bool AddArchive(const stdfs::path& file_path, bool required);

        bool HasFile(const stdfs::path& inner_path) const;
        [[nodiscard]] std::vector<uint8> ReadFile(const stdfs::path& inner_path) const;

    private:
        std::vector<u_ptr<MPQArchive>> archives_;
    };
}


