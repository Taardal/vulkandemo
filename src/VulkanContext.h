#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>

namespace vulkandemo
{
    class VulkanContext
    {
    public:
        struct Config
        {
            std::string Name;
            uint32_t MajorVersion = 1;
            uint32_t MinorVersion = 0;
            uint32_t PatchVersion = 0;
            bool ValidationLayersEnabled = false;
        };

        explicit VulkanContext(const Config& config);

        bool Initialize();

        void Terminate();

    private:
        std::vector<const char*> GetExtensions() const;

        std::vector<const char*> GetRequiredExtensions() const;

        std::vector<VkExtensionProperties> GetAvailableExtensions() const;

        bool HasExtensions(const std::vector<const char*>& extensions, const std::vector<VkExtensionProperties>& availableExtensions) const;

        std::vector<const char*> GetValidationLayers() const;

        std::vector<VkLayerProperties> GetAvailableValidationLayers() const;

        bool HasValidationLayers(const std::vector<const char*>& validationLayers, const std::vector<VkLayerProperties>& availableValidationLayers) const;

    private:
        Config config;
        VkInstance instance;
    };
}