#pragma once
#include <Engine/Core/CoreMinimal.h>


namespace engine
{
	class MPQArchive
	{
	public:
		MPQArchive();
		~MPQArchive();

		bool OpenMPQFile(const stdfs::path& file_path);

		bool HasInnerFile(const stdfs::path& mpq_inner_file_path) const;
		[[nodiscard]] std::vector<uint8> OpenInnerFile(const stdfs::path& mpq_inner_file_path);
		[[nodiscard]] std::vector<uint8> ExtractMapData();
	private:
		void ErrMessage(std::string err_msg) const;
		HANDLE mpq_handle_ = nullptr;
	};
}