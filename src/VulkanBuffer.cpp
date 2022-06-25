#include "VulkanBuffer.h"
#include "Log.h"

namespace Vulkandemo {

    VulkanBuffer::VulkanBuffer(VulkanPhysicalDevice* vulkanPhysicalDevice, VulkanDevice* vulkanDevice) : vulkanPhysicalDevice(vulkanPhysicalDevice), vulkanDevice(vulkanDevice) {
    }

    const VulkanBuffer::Config& VulkanBuffer::getConfig() const {
        return config;
    }

    VkBuffer VulkanBuffer::getBuffer() const {
        return buffer;
    }

    VkDeviceMemory VulkanBuffer::getDeviceMemory() const {
        return deviceMemory;
    }

    bool VulkanBuffer::initialize(const Config& config) {
        this->config = config;

        VkAllocationCallbacks* allocator = VK_NULL_HANDLE;

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = config.Size;
        bufferInfo.usage = config.Usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(vulkanDevice->getDevice(), &bufferInfo, allocator, &buffer) != VK_SUCCESS) {
            VD_LOG_ERROR("Could not create Vulkan buffer");
            return false;
        }

        VkMemoryRequirements memoryRequirements;
        vkGetBufferMemoryRequirements(vulkanDevice->getDevice(), buffer, &memoryRequirements);

        VkMemoryAllocateInfo memoryAllocateInfo{};
        memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memoryAllocateInfo.allocationSize = memoryRequirements.size;
        memoryAllocateInfo.memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, config.MemoryProperties);

        if (vkAllocateMemory(vulkanDevice->getDevice(), &memoryAllocateInfo, allocator, &deviceMemory) != VK_SUCCESS) {
            VD_LOG_ERROR("Could not allocate Vulkan buffer memory");
            return false;
        }

        constexpr VkDeviceSize memoryOffset = 0;
        vkBindBufferMemory(vulkanDevice->getDevice(), buffer, deviceMemory, memoryOffset);

        return true;
    }

    void VulkanBuffer::terminate() {
        VkAllocationCallbacks* allocator = VK_NULL_HANDLE;
        vkDestroyBuffer(vulkanDevice->getDevice(), buffer, allocator);
        vkFreeMemory(vulkanDevice->getDevice(), deviceMemory, allocator);
    }

    void VulkanBuffer::copyTo(const VulkanBuffer& destinationBuffer, const VulkanCommandBuffer& commandBuffer) const {
        VkBufferCopy copyRegion{};
        copyRegion.size = config.Size;
        constexpr uint32_t regionCount = 1;
        vkCmdCopyBuffer(commandBuffer.getCommandBuffer(), buffer, destinationBuffer.buffer, regionCount, &copyRegion);
    }

    uint32_t VulkanBuffer::findMemoryType(uint32_t memoryTypeBits, VkMemoryPropertyFlags memoryPropertyFlags) const {
        VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
        vkGetPhysicalDeviceMemoryProperties(vulkanPhysicalDevice->getPhysicalDevice(), &physicalDeviceMemoryProperties);

        for (uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; i++) {
            // The 'memoryTypeBits' parameter will be used to specify the bit field of memory types that are suitable.
            // That means that we can find the index of a suitable memory type by simply iterating over them and checking if the corresponding bit is set to 1.
            bool isCorrectType = (memoryTypeBits & (1 << i)) == 1;

            // We may have more than one desirable property, so we should check if the result of the bitwise AND is not just non-zero, but equal to the desired properties bit field.
            bool hasCorrectProperties = (physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & memoryPropertyFlags) == memoryPropertyFlags;

            // If there is a memory type suitable for the buffer that also has all of the properties we need, then we return its index.
            if (isCorrectType && hasCorrectProperties) {
                return i;
            }
        }
        VD_LOG_ERROR("Could not find memory type [{}]", memoryTypeBits);
        return -1;
    }
}