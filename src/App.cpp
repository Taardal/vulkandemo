#include "App.h"
#include "Log.h"

#include <vulkan/vulkan.h>

namespace Vulkandemo {

    const int MAX_FRAMES_IN_FLIGHT = 2;

    App::App(Config config)
            : config(std::move(config)),
              fileSystem(new FileSystem),
              window(new Window(config.Window)),
              vulkan(new Vulkan(config.Vulkan, window)),
              vulkanPhysicalDevice(new VulkanPhysicalDevice(vulkan)),
              vulkanDevice(new VulkanDevice(vulkan, vulkanPhysicalDevice)),
              vulkanCommandPool(new VulkanCommandPool(vulkanPhysicalDevice, vulkanDevice)),
              vulkanSwapChain(new VulkanSwapChain(vulkanDevice, vulkanPhysicalDevice, vulkan, window)),
              vulkanRenderPass(new VulkanRenderPass(vulkanSwapChain, vulkanDevice)),
              vulkanGraphicsPipeline(new VulkanGraphicsPipeline(vulkanRenderPass, vulkanSwapChain, vulkanDevice)),
              vertexShader(new VulkanShader(vulkanDevice)),
              fragmentShader(new VulkanShader(vulkanDevice)) {
    }

    App::~App() {
        delete vulkanCommandPool;
        delete vulkanGraphicsPipeline;
        delete vulkanRenderPass;
        delete fragmentShader;
        delete vertexShader;
        delete vulkanSwapChain;
        delete vulkanDevice;
        delete vulkanPhysicalDevice;
        delete vulkan;
        delete window;
        delete fileSystem;
    }

    void App::run() {
        if (!initialize()) {
            VD_LOG_CRITICAL("Could not initialize app");
            return;
        }
        VD_LOG_DEBUG("Running...");
        while (!window->shouldClose()) {
            window->pollEvents();
            drawFrame();
        }
        vulkanDevice->waitUntilIdle();
        terminate();
    }

    bool App::initialize() {
        Log::initialize(config.Name, config.LogLevel);
        VD_LOG_DEBUG("Initializing...");

        if (!window->initialize()) {
            VD_LOG_ERROR("Could not initialize window");
            return false;
        }
        window->setOnResize([this](int width, int height) {
            this->windowResized = true;
        });
        window->setOnMinimize([this](bool minimized) {
            this->windowResized = true;
        });

        if (!vulkan->initialize()) {
            VD_LOG_ERROR("Could not initialize Vulkan");
            return false;
        }
        if (!vulkanPhysicalDevice->initialize()) {
            VD_LOG_ERROR("Could not initialize Vulkan physical device");
            return false;
        }
        if (!vulkanDevice->initialize()) {
            VD_LOG_ERROR("Could not initialize Vulkan device");
            return false;
        }
        if (!vertexShader->initialize(fileSystem->readBytes("shaders/simple_shader.vert.spv"))) {
            VD_LOG_ERROR("Could not initialize vertex shader");
            return false;
        }
        if (!fragmentShader->initialize(fileSystem->readBytes("shaders/simple_shader.frag.spv"))) {
            VD_LOG_ERROR("Could not initialize fragment shader");
            return false;
        }
        if (!vulkanCommandPool->initialize()) {
            VD_LOG_ERROR("Could not initialize Vulkan command pool");
            return false;
        }
        if (!initializeSwapChain()) {
            VD_LOG_ERROR("Could not initialize Vulkan swap chain");
            return false;
        }
        return true;
    }

    bool App::initializeSwapChain() {
        if (!vulkanSwapChain->initialize()) {
            VD_LOG_ERROR("Could not initialize Vulkan swap chain");
            return false;
        }
        if (!vulkanRenderPass->initialize()) {
            VD_LOG_ERROR("Could not initialize Vulkan render pass");
            return false;
        }
        if (!vulkanGraphicsPipeline->initialize(*vertexShader, *fragmentShader)) {
            VD_LOG_ERROR("Could not initialize Vulkan graphics pipeline");
            return false;
        }
        if (!vulkanRenderer->initialize()) {
            VD_LOG_ERROR("Could not create Vulkan command buffers");
            return false;
        }
        return true;
    }

    void App::terminate() {
        VD_LOG_DEBUG("Terminating...");
        terminateSwapChain();
        vulkanCommandPool->terminate();
        fragmentShader->terminate();
        vertexShader->terminate();
        vulkanDevice->terminate();
        vulkan->terminate();
        window->terminate();
    }

    void App::terminateSwapChain() {
        vulkanRenderer->terminate();
        vulkanGraphicsPipeline->terminate();
        vulkanRenderPass->terminate();
        vulkanSwapChain->terminate();
    }

    bool App::recreateSwapChain() {
        window->waitUntilNotMinimized();
        vulkanDevice->waitUntilIdle();
        terminateSwapChain();
        vulkanPhysicalDevice->updateSwapChainInfo();
        return initializeSwapChain();
    }
}