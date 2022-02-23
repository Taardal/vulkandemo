#pragma once

#include "Command.h"
#include "FileSystem.h"
#include <string>
#include <sstream>

namespace VulkandemoCLI
{
    class InstallDependenciesCommand : public Command
    {
    public:
        InstallDependenciesCommand(FileSystem* fileSystem);

        const char* GetName() const override;

        const char* GetDescription() const override;

        void Execute(const std::vector<Option>& options) const override;

    public:
        static const char* NAME;
        static const char* DESCRIPTION;

    private:
        static const char* DEPENDENCIES_FILENAME;
        static const char* DEPENDENCIES_DIRECTORY_NAME;

        FileSystem* fileSystem;
    };
}
