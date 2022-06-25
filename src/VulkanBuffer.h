#pragma once

#include "VulkanDevice.h"
#include "VulkanCommandBuffer.h"

#include <vulkan/vulkan.h>

namespace Vulkandemo {

    class VulkanBuffer {
    public:
        struct Config {
            VkDeviceSize Size;
            VkBufferUsageFlags Usage;
            VkMemoryPropertyFlags MemoryProperties;
        };

    private:
        VulkanPhysicalDevice* vulkanPhysicalDevice;
        VulkanDevice* vulkanDevice;
        Config config;
        VkBuffer buffer;
        VkDeviceMemory deviceMemory;

    public:
        VulkanBuffer(VulkanPhysicalDevice* vulkanPhysicalDevice, VulkanDevice* vulkanDevice);

        const Config& getConfig() const;

        VkBuffer getBuffer() const;

        VkDeviceMemory getDeviceMemory() const;

        bool initialize(const Config& config);

        void terminate();

        void copyTo(const VulkanBuffer& destinationBuffer, const VulkanCommandBuffer& commandBuffer) const;

    private:
        uint32_t findMemoryType(uint32_t memoryTypeBits, VkMemoryPropertyFlags memoryPropertyFlags) const;
    };

}