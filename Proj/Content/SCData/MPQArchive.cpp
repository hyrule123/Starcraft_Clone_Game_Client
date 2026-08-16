#include "Content/pch.h"
#include "MPQArchive.h"

#if defined(_WIN64) && defined(_DEBUG)
#pragma comment (lib, "StormLib/x64/DebugUD/StormLibDUD.lib")
#endif

#if defined(_WIN64) && defined(_NDEBUG)
#pragma comment (lib, "StormLib/x64/Release/StormLibRUD.lib")
#endif

#ifndef _WIN64
#error("x64만 지원")
#endif

#include <StormLib/Include/StormLib.h>

#include <Engine/Core/Debug.h>

namespace engine
{


	MPQArchive::MPQArchive()
	{}
	MPQArchive::~MPQArchive()
	{
		if (mpq_handle_) 
		{ 
			SFileCloseArchive(mpq_handle_); 
			mpq_handle_ = {}; 
		}
	}
	bool MPQArchive::OpenMPQFile(const stdfs::path & file_path)
	{
		//파일 로드
		bool result = SFileOpenArchive(file_path.wstring().c_str(), 0, 0, &mpq_handle_);
		if (result == false)
		{
			ErrMessage("Failed to open MPQ archive.");
			return false;
		}
		return true;
	}
	std::vector<uint8> MPQArchive::OpenInnerFile(const stdfs::path& mpq_inner_file_path)
	{
		if(!mpq_handle_)
		{
			ERROR_MESSAGE("MPQ archive is not opened.");
			return {};
		}

		struct MPQInnerFileHandle // Archived file handle RAII
		{
			HANDLE handle = nullptr;
			~MPQInnerFileHandle() { if (handle) SFileCloseFile(handle); }
		} inner_file = {};

		// MPQ 파일 내부에서 파일 탐색
		bool result = SFileOpenFileEx(mpq_handle_, mpq_inner_file_path.string().c_str(), 0, &inner_file.handle);
		if (result == false)
		{
			ErrMessage("Failed to open " + mpq_inner_file_path.string() + " file.");
			return {};
		}

		// 파일 크기 받아서 버퍼 할당
		DWORD file_size = SFileGetFileSize(inner_file.handle, NULL);
		if (file_size == 0 || file_size == SFILE_INVALID_SIZE)
		{
			ErrMessage("Failed to get file size or file is empty.");
			return {};
		}

		std::vector<uint8> data((size_t)file_size, 0);

		//파일 읽기
		DWORD actual_read_bytes = 0;
		result = SFileReadFile(inner_file.handle, data.data(), file_size, &actual_read_bytes, NULL);
		if (result == false)
		{
			ErrMessage("Failed to read file.");
			return {};
		}

		//검증
		ASSERT(0 < actual_read_bytes && actual_read_bytes <= file_size);

		//실제로 읽어들인 만큼 데이터 축소
		data.resize(actual_read_bytes);

		//파일 닫기 - 생략 (자동으로 RAII 구조체에서 처리됨)

		return data;
	}
	std::vector<uint8> MPQArchive::ExtractMapData()
	{
		return OpenInnerFile("staredit\\scenario.chk");
	}
	void MPQArchive::ErrMessage(std::string err_msg) const
	{
		err_msg += "\nErrCode: " + std::to_string(GetLastError());
		ERROR_MESSAGE_A(err_msg.c_str());
	}
}