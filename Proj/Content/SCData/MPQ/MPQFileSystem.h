#pragma once
#include <Engine/Core/CoreMinimal.h>
#include <Content/SCData/MPQ/MPQFile.h>

namespace engine
{
    class MPQArchive;

    // 우선순위 순서로 MPQ를 보관하고, 먼저 발견된 파일을 사용 (OpenBW와 동일 규칙)
    class MPQFileSystem
    {
    public:
        // 추가 순서 = 검색 우선순위. required가 false면 파일이 없어도 성공
        bool AddArchive(const stdfs::path& file_path, bool required);

        bool HasFile(const stdfs::path& inner_path) const;

        [[nodiscard]] MPQFile OpenFile(const stdfs::path& inner_path) const;

		template <typename T = uint8> requires std::is_trivially_copyable_v<T>
		[[nodiscard]] std::optional<std::vector<T>> ReadFile(const stdfs::path& inner_path) const
		{
			MPQFile file = OpenFile(inner_path);
			if (!file.IsValid())
			{
				return std::nullopt;
			}
			return file.ReadAll<T>();
		}

		template  <typename T = uint8> requires std::is_trivially_copyable_v<T>
		[[nodiscard]] bool ReadFile(const stdfs::path& inner_path, std::vector<T>& out_data) const
		{
			out_data.clear();
			MPQFile file = OpenFile(inner_path);
			if (!file.IsValid())
			{
				return false;
			}
			return file.ReadAll(out_data);
		}

    private:
        std::vector<u_ptr<MPQArchive>> archives_;
    };
}


