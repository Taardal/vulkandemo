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
#include "VulkanCommandPool.h"
#include "VulkanRenderer.h"

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
        VulkanCommandPool* vulkanCommandPool;
        VulkanRenderer* vulkanRenderer;

    public:
        explicit App(Config config);

        ~App();

        void run();

    private:
        bool initialize();

        bool initializeSwapChain();

        void terminate();

        void terminateSwapChain();

        bool recreateSwapChain();
    };

}