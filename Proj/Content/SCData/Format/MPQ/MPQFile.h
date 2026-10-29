#pragma once
#include <Engine/Core/CoreMinimal.h>
#include <Engine/Core/Debug.h>
#include <Engine/Core/Constants.h>

#include <optional>

namespace sc
{
	void MPQErrMessage(std::string err_msg);

	// MPQ 내부 파일 핸들 RAII
	// 아카이브가 먼저 닫혀도 안전 (StormLib이 열린 파일 수로 아카이브 해제를 지연)
	//
	// ReadAt / ReadAll은 매번 지정 위치로 이동한 뒤 읽으므로 호출 순서와 무관하게 같은 결과 → const
	// 단, 내부적으로 StormLib의 파일 위치를 바꾸므로 스레드 안전하지 않음
	// (같은 MPQFile을 여러 스레드에서 동시에 사용 금지)
	class MPQFile
	{
		friend class MPQArchive;
	public:
		static constexpr size_t kInvalidSize = engine::kInvalidSize;

		MPQFile() = default;
		MPQFile(const MPQFile&) = delete;
		MPQFile& operator=(const MPQFile&) = delete;
		MPQFile(MPQFile&&) noexcept;
		MPQFile& operator=(MPQFile&&) noexcept;
		~MPQFile();

		// 실패 시 kInvalidSize
		size_t Size() const;

		// offset 위치부터 size 바이트를 읽음
		// 범위(offset + size)가 파일 크기를 넘으면 실패
		// 반환: 읽은 바이트 수, 실패 시 kInvalidSize
		size_t ReadAt(size_t offset, void* dest, size_t size) const;

		// 파일 전체를 T 배열로 읽음 (레코드가 연속된 AoS 파일 전용, images.dat 같은 SoA 파일은 불가)
		// 크기 0인 파일, 크기가 sizeof(T)의 배수가 아닌 파일은 실패
		// 실패 시 out_data는 항상 비워짐
		template <typename T = uint8_t> requires std::is_trivially_copyable_v<T>
		[[nodiscard]] bool ReadAll(std::vector<T>& out_data) const
		{
			out_data.clear();

			const size_t file_size = Size();
			if (file_size == kInvalidSize)
			{
				return false;  // Size가 이미 알림
			}
			if (file_size == 0)
			{
				ERR_MSG("MPQFile: File size is 0");
				return false;
			}
			if (file_size % sizeof(T) != 0)
			{
				ERR_MSG("MPQFile: File size({}) is not a multiple of type size({})", file_size, sizeof(T));
				return false;
			}

			out_data.resize(file_size / sizeof(T));
			if (ReadAt(0, out_data.data(), file_size) != file_size)
			{
				out_data.clear();
				return false;  // ReadAt이 이미 알림
			}
			return true;
		}

		// 실패 시 빈 vector (ReadAll(out_data)와 같은 실패 조건)
		template <typename T = uint8_t> requires std::is_trivially_copyable_v<T>
		std::optional<std::vector<T>> ReadAll() const
		{
			std::vector<T> out_data;
			if (!ReadAll(out_data))
			{
				return std::nullopt;
			}
			return out_data;
		}

		bool IsValid() const { return handle_ != nullptr; }

		void Close();

	private:
		void* handle_ = {};
	};
}