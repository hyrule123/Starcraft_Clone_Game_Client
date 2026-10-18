#pragma once
#include <Engine/Core/CoreMinimal.h>

#include <Content/SCData/MPQ/MPQFile.h>

namespace sc
{
	class MPQArchive
	{
	public:
		MPQArchive();
		~MPQArchive();

		bool OpenMPQFile(const std::filesystem::path& file_path);
		bool HasInnerFile(const std::filesystem::path& mpq_inner_file_path) const;
		[[nodiscard]] MPQFile OpenInnerFile(const std::filesystem::path& mpq_inner_file_path) const;

		template  <typename T = uint8_t> requires std::is_trivially_copyable_v<T>
		[[nodiscard]] std::optional<std::vector<T>> ReadInnerFile(const std::filesystem::path& mpq_inner_file_path) const
		{
			MPQFile file = OpenInnerFile(mpq_inner_file_path);
			if (!file.IsValid())
			{
				return std::nullopt;
			}
			return file.ReadAll<T>();
		}

		template <typename T = uint8_t> requires std::is_trivially_copyable_v<T>
		[[nodiscard]] bool ReadInnerFile(const std::filesystem::path& mpq_inner_file_path, std::vector<T>& out_data) const
		{
			out_data.clear();
			MPQFile file = OpenInnerFile(mpq_inner_file_path);
			if (!file.IsValid())
			{
				return false;
			}
			return file.ReadAll(out_data);
		}

	private:
		MPQFile archive_mpq_ = {};
	};
}
