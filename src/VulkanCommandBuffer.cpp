#include "VulkanCommandBuffer.h"
#include "Log.h"

namespace Vulkandemo {

    const VkAllocationCallbacks* VulkanCommandBuffer::ALLOCATOR = VK_NULL_HANDLE;

    VulkanCommandBuffer::VulkanCommandBuffer(VkCommandBuffer commandBuffer) : commandBuffer(commandBuffer) {
    }

    const VkCommandBuffer VulkanCommandBuffer::getCommandBuffer() const {
        return commandBuffer;
    }

    bool VulkanCommandBuffer::begin(VkCommandBufferUsageFlags flags) const {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = flags;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            VD_LOG_ERROR("Could not begin Vulkan command buffer");
            return false;
        }
        return true;
    }

    bool VulkanCommandBuffer::end() const {
        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            VD_LOG_ERROR("Could not end Vulkan command buffer");
            return false;
        }
        return true;
    }

    void VulkanCommandBuffer::reset() const {
        VkCommandBufferResetFlags flags = 0;
        vkResetCommandBuffer(commandBuffer, flags);
    }

}