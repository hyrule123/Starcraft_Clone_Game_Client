#pragma once
#include <Engine/Core/CoreMinimal.h>
#include <Content/SCData/Format/MPQ/MPQFile.h>

namespace sc
{
	class MPQArchive
	{
	public:
		MPQArchive();
		MPQArchive(const MPQArchive&) = delete;
		MPQArchive& operator=(const MPQArchive&) = delete;
		MPQArchive(MPQArchive&&) noexcept;
		MPQArchive& operator=(MPQArchive&&) noexcept;
		~MPQArchive();

		bool OpenArchive(const std::filesystem::path& file_path);
		void CloseArchive();

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
		void* archive_handle_ = {};
	};
}
