#include "Content/pch.h"
#include "MPQFileSystem.h"

#include <Content/SCData/MPQArchive.h>

#include <Engine/Core/Debug.h>
#include <Engine/Core/Constants.h>

namespace engine
{
    bool MPQFileSystem::AddArchive(const stdfs::path& file_path, bool required)
    {
        if (!required && !stdfs::exists(file_path))
        {
            return true;  // 선택 아카이브가 없는 건 정상
        }

        u_ptr<MPQArchive> archive = std::make_unique<MPQArchive>();
        if (!archive->OpenMPQFile(file_path))
        {
            return false;  // 필수가 없거나, 선택이라도 존재하는데 손상된 경우
        }

        archives_.push_back(std::move(archive));
        return true;
    }

    bool MPQFileSystem::HasFile(const stdfs::path& inner_path) const
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

    std::vector<uint8> MPQFileSystem::ReadFile(const stdfs::path& inner_path) const
    {
        for(const auto& archive : archives_)
        {
            if (archive->HasInnerFile(inner_path))
            {
                return archive->OpenInnerFile(inner_path);
            }
        }

        return {};
    }
}


