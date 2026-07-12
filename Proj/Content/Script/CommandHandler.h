#pragma once
#include <Engine/Game/Component/Script.h>

#include <Engine/Core/CoreMinimal.h>

namespace engine
{
    class CommandHandler :
        public Script
    {
        CLASS_INFO(CommandHandler, Script)
    public:
        CommandHandler();
		virtual ~CommandHandler() override;

    private:

    };
}

