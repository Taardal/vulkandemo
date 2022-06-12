#pragma once

#include "Log.h"
#include "FileSystem.h"
#include "Window.h"
#include "Vulkan.h"
#include "VulkanPhysicalDevice.h"
#include "VulkanDevice.h"
#include "VulkanCommandPool.h"
#include "VulkanCommandBuffer.h"
#include "VulkanSwapChain.h"
#include "VulkanRenderPass.h"
#include "VulkanGraphicsPipeline.h"
#include "VulkanFramebuffer.h"
#include "Vertex.h"

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
        VulkanCommandPool* vulkanCommandPool;
        std::vector<VulkanCommandBuffer> vulkanCommandBuffers;
        VulkanShader* vertexShader;
        VulkanShader* fragmentShader;
        VulkanSwapChain* vulkanSwapChain;
        VulkanRenderPass* vulkanRenderPass;
        VulkanGraphicsPipeline* vulkanGraphicsPipeline;
        std::vector<VulkanFramebuffer> framebuffers;
        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;
        uint32_t currentFrame = 0;
        bool windowResized = false;

        VkBuffer vertexBuffer;
        VkDeviceMemory vertexBufferDeviceMemory;
        VkBuffer indexBuffer;
        VkDeviceMemory indexBufferDeviceMemory;

        const std::vector<Vertex> vertices = {
                {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
                {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
                {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
                {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
        };

        const std::vector<uint16_t> indices = {
                0, 1, 2, 2, 3, 0
        };

    public:
        explicit App(Config config);

        ~App();

        void run();

    private:
        bool initialize();

        bool initializeIndexBuffer();

        bool initializeVertexBuffer();

        bool createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties, VkBuffer& buffer, VkDeviceMemory& bufferDeviceMemory);

        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags propertyFlags) const;

        void terminateIndexBuffer();

        void terminateVertexBuffer();

        bool initializeRenderingObjects();

        bool initializeFramebuffers();

        bool initializeSyncObjects();

        void terminate();

        void terminateSyncObjects() const;

        void terminateFramebuffers();

        void terminateRenderingObjects();

        bool recreateRenderingObjects();

        void drawFrame();

        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const;
    };

}