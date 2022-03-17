#pragma once

#include "Vulkan.h"

#include <vulkan/vulkan.h>
#include <optional>
#include <vector>

namespace Vulkandemo {

    struct QueueFamilyIndices {
        std::optional<uint32_t> GraphicsFamily;
        std::optional<uint32_t> PresentationFamily;
    };

}

namespace Vulkandemo {

    class VulkanPhysicalDevice {
    private:
        struct DeviceInfo {
            VkPhysicalDevice VkDevice = nullptr;
            VkPhysicalDeviceProperties VkDeviceProperties{};
            VkPhysicalDeviceFeatures VkDeviceFeatures{};
            QueueFamilyIndices QueueFamilyIndices;
        };

    private:
        Vulkan* vulkan;
        VkPhysicalDevice vkPhysicalDevice = VK_NULL_HANDLE;
        QueueFamilyIndices queueFamilies{};
        DeviceInfo deviceInfo{};

    public:
        explicit VulkanPhysicalDevice(Vulkan* vulkan);

        VkPhysicalDevice getVkPhysicalDevice() const;

        const VkPhysicalDeviceFeatures& getVkDeviceFeatures() const;

        const QueueFamilyIndices& getQueueFamilyIndices() const;

        bool initialize();

    private:
        std::vector<DeviceInfo> findAvailableDevices() const;

        std::string getDeviceTypeAsString(VkPhysicalDeviceType deviceType) const;

        DeviceInfo findMostEligibleDevice(const std::vector<DeviceInfo>& availableDevices) const;

        QueueFamilyIndices findQueueFamilyIndices(VkPhysicalDevice device) const;

        int getRating(const DeviceInfo& deviceInfo) const;
    };

}

