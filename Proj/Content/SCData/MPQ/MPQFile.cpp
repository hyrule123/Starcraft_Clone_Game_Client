#include "Content/pch.h"
#include "MPQFile.h"

#if defined(_WIN64) && defined(_DEBUG)
#pragma comment (lib, "StormLib/x64/DebugUD/StormLibDUD.lib")
#endif

#if defined(_WIN64) && defined(NDEBUG)
#pragma comment (lib, "StormLib/x64/ReleaseUD/StormLibRUD.lib")
#endif

#ifndef _WIN64
#error("Only x64 is supported")
#endif

#include <StormLib/Include/StormLib.h>

namespace engine
{
	void MPQErrMessage(std::string err_msg)
	{
		ERR_MSG("{}\nErrCode: {}", err_msg, GetLastError());
	}

	MPQFile::MPQFile(MPQFile&& other) noexcept
		: handle_(std::exchange(other.handle_, nullptr))
	{}

	MPQFile& MPQFile::operator=(MPQFile&& other) noexcept
	{
		if (this != &other)
		{
			Close();
			handle_ = std::exchange(other.handle_, nullptr);
		}
		return *this;
	}

	MPQFile::~MPQFile()
	{
		Close();
	}

	size_t MPQFile::Size() const
	{
		if (!handle_)
		{
			ERR_MSG("MPQFile handle is null.");
			return kInvalidSize;
		}

		// DWORD 상태로 비교해야 함
		// (size_t로 먼저 캐스팅하면 64비트에서 0x00000000FFFFFFFF가 되어 kInvalidSize와 달라짐)
		const DWORD file_size = SFileGetFileSize(handle_, nullptr);
		if (file_size == SFILE_INVALID_SIZE)
		{
			MPQErrMessage("Failed to get inner file size.");
			return kInvalidSize;
		}
		return static_cast<size_t>(file_size);
	}

	size_t MPQFile::ReadAt(size_t offset, void* dest, size_t size) const
	{
		if (!handle_)
		{
			ERR_MSG("MPQFile handle is null.");
			return kInvalidSize;
		}

		ASSERT(dest != nullptr || size == 0);
		ASSERT(size <= MAXDWORD);  // MPQ 파일은 4GB를 넘지 않음

		// SFileSetFilePointer는 파일 끝을 넘는 이동을 에러 없이 끝으로 잘라내므로 범위를 먼저 검사
		const size_t file_size = Size();
		if (file_size == kInvalidSize)
		{
			return kInvalidSize;
		}
		if (offset > file_size || size > file_size - offset)
		{
			ERR_MSG("MPQFile: 읽기 범위가 파일 크기를 벗어남 (offset {}, size {}, file {})", offset, size, file_size);
			return kInvalidSize;
		}

		if (SFileSetFilePointer(handle_, static_cast<LONG>(offset), nullptr, FILE_BEGIN) == SFILE_INVALID_POS)
		{
			MPQErrMessage("Failed to seek inner file.");
			return kInvalidSize;
		}

		DWORD actual_read_bytes = 0;
		if (!SFileReadFile(handle_, dest, static_cast<DWORD>(size), &actual_read_bytes, nullptr))
		{
			MPQErrMessage("Failed to read inner file.");
			return kInvalidSize;
		}
		return static_cast<size_t>(actual_read_bytes);
	}

	void MPQFile::Close()
	{
		if (handle_)
		{
			SFileCloseFile(handle_);
			handle_ = nullptr;
		}
	}
}