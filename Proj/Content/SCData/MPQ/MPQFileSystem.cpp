#include "Content/pch.h"
#include "MPQFileSystem.h"

#include <Content/SCData/MPQ/MPQArchive.h>

#include <Engine/Core/Debug.h>
#include <Engine/Core/Constants.h>

namespace sc
{
    bool MPQFileSystem::AddArchive(const std::filesystem::path& file_path, bool required)
    {
        if (!required && !std::filesystem::exists(file_path))
        {
            return true;  // 선택 아카이브가 없는 건 정상
        }

        std::unique_ptr<MPQArchive> archive = std::make_unique<MPQArchive>();
        if (!archive->OpenMPQFile(file_path))
        {
            return false;  // 필수가 없거나, 선택이라도 존재하는데 손상된 경우
        }

        archives_.push_back(std::move(archive));
        return true;
    }

    bool MPQFileSystem::HasFile(const std::filesystem::path& inner_path) const
    {
		for (const auto& archive : archives_)
		{
			if (archive->HasInnerFile(inner_path))
			{
				return true;
			}
		}

        return false;
    }

    MPQFile MPQFileSystem::OpenFile(const std::filesystem::path& inner_path) const
    {
		for (const auto& archive : archives_)
		{
            ASSERT(archive);
			if (archive->HasInnerFile(inner_path))
			{
				return archive->OpenInnerFile(inner_path);
			}
		}

        return MPQFile();
    }

}


