#pragma once

#include "VulkanPhysicalDevice.h"
#include "VulkanDevice.h"
#include "VulkanCommandBuffer.h"

#include <vulkan/vulkan.h>

namespace Vulkandemo {

    class VulkanCommandPool {
    private:
        static const VkAllocationCallbacks* ALLOCATOR;

    private:
        VulkanPhysicalDevice* vulkanPhysicalDevice;
        VulkanDevice* vulkanDevice;
        VkCommandPool commandPool = VK_NULL_HANDLE;

    public:
        VulkanCommandPool(VulkanPhysicalDevice* vulkanPhysicalDevice, VulkanDevice* vulkanDevice);

        const VkCommandPool getCommandPool() const;

        bool initialize();

        void terminate();

        std::vector<VulkanCommandBuffer> allocateCommandBuffers(uint32_t count) const;
    };

}
