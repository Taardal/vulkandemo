#pragma once

#include "VulkanDevice.h"
#include "VulkanSwapChain.h"
#include "VulkanRenderPass.h"
#include "VulkanGraphicsPipeline.h"
#include "VulkanCommandPool.h"
#include "VulkanCommandBuffer.h"
#include "VulkanFramebuffer.h"

#include <vulkan/vulkan.h>
#include <vector>

namespace Vulkandemo {

    class VulkanRenderer {
    private:
        static const uint32_t MAX_FRAMES_IN_FLIGHT;

    private:
        VulkanDevice* vulkanDevice;
        VulkanSwapChain* vulkanSwapChain;
        VulkanRenderPass* vulkanRenderPass;
        VulkanGraphicsPipeline* vulkanGraphicsPipeline;
        VulkanCommandPool* vulkanCommandPool;
        std::vector<VulkanCommandBuffer> vulkanCommandBuffers;
        std::vector<VulkanFramebuffer> framebuffers;
        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;
        uint32_t currentFrame = 0;
        bool windowResized = false;

    public:
        bool initialize();

        void terminate();

        void drawFrame();

    private:
        bool initializeFramebuffers();

        bool initializeSyncObjects();

        void terminateSyncObjects();

        void terminateFramebuffers();
    };

}
