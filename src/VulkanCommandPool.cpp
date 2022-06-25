#include "VulkanCommandPool.h"
#include "Log.h"

namespace Vulkandemo {

    const VkAllocationCallbacks* VulkanCommandPool::ALLOCATOR = VK_NULL_HANDLE;

    VulkanCommandPool::VulkanCommandPool(VulkanPhysicalDevice* vulkanPhysicalDevice, VulkanDevice* vulkanDevice) : vulkanPhysicalDevice(vulkanPhysicalDevice), vulkanDevice(vulkanDevice) {
    }

    const VkCommandPool VulkanCommandPool::getCommandPool() const {
        return commandPool;
    }

    bool VulkanCommandPool::initialize() {
        VkCommandPoolCreateInfo commandPoolInfo{};
        commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        commandPoolInfo.queueFamilyIndex = vulkanPhysicalDevice->getQueueFamilyIndices().GraphicsFamily.value();

        if (vkCreateCommandPool(vulkanDevice->getDevice(), &commandPoolInfo, ALLOCATOR, &commandPool) != VK_SUCCESS) {
            VD_LOG_ERROR("Could not create Vulkan command pool");
            return false;
        }
        VD_LOG_INFO("Created Vulkan command pool");

        return true;
    }

    void VulkanCommandPool::terminate() {
        vkDestroyCommandPool(vulkanDevice->getDevice(), commandPool, ALLOCATOR);
        VD_LOG_INFO("Destroyed Vulkan command pool");
    }

    VulkanCommandBuffer VulkanCommandPool::allocateCommandBuffer() const {
        VkCommandBufferAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandPool = commandPool;
        allocateInfo.commandBufferCount = 1;

        VkCommandBuffer vkCommandBuffer;
        if (vkAllocateCommandBuffers(vulkanDevice->getDevice(), &allocateInfo, &vkCommandBuffer) != VK_SUCCESS) {
            VD_LOG_ERROR("Could not allocate Vulkan command buffer");
        }

        VulkanCommandBuffer vulkanCommandBuffer(vkCommandBuffer);
        return vulkanCommandBuffer;
    }

    std::vector<VulkanCommandBuffer> VulkanCommandPool::allocateCommandBuffers(uint32_t count) const {
        std::vector<VkCommandBuffer> vkCommandBuffers;
        vkCommandBuffers.resize(count);

        VkCommandBufferAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandBufferCount = vkCommandBuffers.size();
        allocateInfo.commandPool = commandPool;

        if (vkAllocateCommandBuffers(vulkanDevice->getDevice(), &allocateInfo, vkCommandBuffers.data()) != VK_SUCCESS) {
            VD_LOG_ERROR("Could not allocate [{}] Vulkan command buffers", vkCommandBuffers.size());
            return {};
        }

        std::vector<VulkanCommandBuffer> vulkanCommandBuffers;
        for (VkCommandBuffer vkCommandBuffer : vkCommandBuffers) {
            VulkanCommandBuffer vulkanCommandBuffer(vkCommandBuffer);
            vulkanCommandBuffers.push_back(vulkanCommandBuffer);
        }
        VD_LOG_INFO("Allocated [{}] command buffers", vulkanCommandBuffers.size());
        return vulkanCommandBuffers;
    }

    void VulkanCommandPool::freeCommandBuffer(const VulkanCommandBuffer& commandBuffer) const {
        constexpr uint32_t commandBufferCount = 1;
        VkCommandBuffer vkCommandBuffer = commandBuffer.getCommandBuffer();
        vkFreeCommandBuffers(vulkanDevice->getDevice(), commandPool, commandBufferCount, &vkCommandBuffer);
    }

}