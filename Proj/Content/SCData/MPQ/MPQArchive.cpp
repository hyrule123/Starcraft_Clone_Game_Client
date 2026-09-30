#include "Content/pch.h"
#include "MPQArchive.h"

#include <StormLib/Include/StormLib.h>

#include <Engine/Core/Debug.h>
#include <Engine/Core/Constants.h>

#include <type_traits>

// StormLib uses HANDLE(= void*), which is defined as void* in the header. Therefore, use static_assert to check type consistency
static_assert(std::is_same_v<HANDLE, void*>, "HANDLE and void* are not the same type. Change the variable in header accordingly.");

namespace sc
{
	MPQArchive::MPQArchive()
	{}
	MPQArchive::~MPQArchive()
	{}

	bool MPQArchive::OpenMPQFile(const std::filesystem::path & file_path)
	{
		//파일 로드
		MPQFile mpq_file;
		bool result = SFileOpenArchive(file_path.wstring().c_str(), 0, 0, &mpq_file.handle_);
		if (result == false)
		{
			MPQErrMessage("Failed to open MPQ archive.");
			return false;
		}
		archive_mpq_ = std::move(mpq_file);
		return true;
	}

	bool MPQArchive::HasInnerFile(const std::filesystem::path& mpq_inner_file_path) const
	{
		return SFileHasFile(archive_mpq_.handle_, mpq_inner_file_path.string().c_str());
	}

	MPQFile MPQArchive::OpenInnerFile(const std::filesystem::path& mpq_inner_file_path) const
	{
		if (archive_mpq_.IsValid() == false)
		{
			ERR_MSG("MPQ archive is not opened.");
			return {};
		}

		MPQFile inner_file;

		// MPQ 파일 내부에서 파일 탐색
		bool result = SFileOpenFileEx(archive_mpq_.handle_, mpq_inner_file_path.string().c_str(), 0, &inner_file.handle_);
		if (result == false)
		{
			MPQErrMessage("Failed to open " + mpq_inner_file_path.string() + " file.");
			return {};
		}

		return inner_file;
	}
}