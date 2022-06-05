#pragma once

#include "VulkanShader.h"
#include "VulkanSwapChain.h"
#include "VulkanDevice.h"
#include "VulkanCommandBuffer.h"

#include <vulkan/vulkan.h>

namespace Vulkandemo {

    class VulkanFramebuffer;

    class VulkanRenderPass {
    private:
        static const VkAllocationCallbacks* ALLOCATOR;

    private:
        VulkanSwapChain* vulkanSwapChain;
        VulkanDevice* vulkanDevice;
        VkRenderPass renderPass = VK_NULL_HANDLE;

    public:
        VulkanRenderPass(VulkanSwapChain* vulkanSwapChain, VulkanDevice* vulkanDevice);

        const VkRenderPass getRenderPass() const;

        bool initialize();

        void terminate();

        void begin(const VulkanCommandBuffer& vulkanCommandBuffer, const VulkanFramebuffer& vulkanFramebuffer) const;

        void end(const VulkanCommandBuffer& vulkanCommandBuffer) const;
    };

}



