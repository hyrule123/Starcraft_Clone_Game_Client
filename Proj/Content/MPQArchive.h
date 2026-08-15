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
		std::vector<uint8> OpenInnerFile(const stdfs::path& mpq_inner_file_path);
		std::vector<uint8> ExtractMapData();
	private:
		void ErrMessage(std::string err_msg) const;
		HANDLE mpq_handle_ = nullptr;
	};
}