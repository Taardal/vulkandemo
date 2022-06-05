#pragma once

#include "Log.h"
#include "FileSystem.h"
#include "Window.h"
#include "Vulkan.h"
#include "VulkanPhysicalDevice.h"
#include "VulkanDevice.h"
#include "VulkanSwapChain.h"
#include "VulkanRenderPass.h"
#include "VulkanGraphicsPipeline.h"
#include "VulkanFramebuffer.h"
#include "VulkanCommandPool.h"
#include "VulkanCommandBuffer.h"

#include <vulkan/vulkan.h>

#include <vector>

namespace Vulkandemo {

    class App {
    public:
        struct Config {
            std::string Name;
            Log::Level LogLevel;
            Window::Config Window;
            Vulkan::Config Vulkan;
        };

    private:
        Config config;
        FileSystem* fileSystem;
        Window* window;
        Vulkan* vulkan;
        VulkanPhysicalDevice* vulkanPhysicalDevice;
        VulkanDevice* vulkanDevice;
        VulkanSwapChain* vulkanSwapChain;
        VulkanShader* vertexShader;
        VulkanShader* fragmentShader;
        VulkanRenderPass* vulkanRenderPass;
        VulkanGraphicsPipeline* vulkanGraphicsPipeline;
        std::vector<VulkanFramebuffer> framebuffers;
        VulkanCommandPool* vulkanCommandPool;
        std::vector<VulkanCommandBuffer> vulkanCommandBuffers;
        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;
        uint32_t currentFrame = 0;
        bool windowResized = false;

    public:
        explicit App(Config config);

        ~App();

        void run();

    private:
        bool initialize();

        bool initializeSwapChain();

        bool initializeCommandBuffers();

        bool initializeSyncObjects();

        void terminate();

        void terminateSyncObjects();

        void terminateSwapChain();

        bool recreateSwapChain();

        void drawFrame();
    };

}