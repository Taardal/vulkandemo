#pragma once

#include <vulkan/vulkan.h>

namespace Vulkandemo {

    class VulkanCommandBuffer {
    private:
        static const VkAllocationCallbacks* ALLOCATOR;

    private:
        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

    public:
        explicit VulkanCommandBuffer(VkCommandBuffer commandBuffer);

        const VkCommandBuffer getCommandBuffer() const;

        bool begin(VkCommandBufferUsageFlags flags = 0) const;

        bool end() const;

        void reset() const;
    };

}
