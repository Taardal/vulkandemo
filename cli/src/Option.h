#pragma once

#include <string>

namespace VulkandemoCLI
{
    struct Option
    {
        std::string Name;
        std::string NameWithDashes;
        std::string Value;
    };
}
