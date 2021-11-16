#pragma once

#include <vector>

namespace Vulkandemo
{
    class FileSystem
    {
    public:
        std::vector<char> ReadBinaryFile(const char* path) const;
    };
}
