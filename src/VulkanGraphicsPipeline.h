#pragma once

#include "VulkanShader.h"
#include "VulkanRenderPass.h"
#include "VulkanSwapChain.h"
#include "VulkanDevice.h"

#include <vulkan/vulkan.h>

namespace Vulkandemo {

    class VulkanGraphicsPipeline {
    private:
        static const VkAllocationCallbacks* ALLOCATOR;

    private:
        VulkanRenderPass* vulkanRenderPass;
        VulkanSwapChain* vulkanSwapChain;
        VulkanDevice* vulkanDevice;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkPipeline pipeline = VK_NULL_HANDLE;

    public:
        VulkanGraphicsPipeline(VulkanRenderPass* vulkanRenderPass, VulkanSwapChain* vulkanSwapChain, VulkanDevice* vulkanDevice);

        bool initialize(const VulkanShader& vertexShader, const VulkanShader& fragmentShader);

        void terminate();

        void bind(const VulkanCommandBuffer& vulkanCommandBuffer) const;
    };

}


