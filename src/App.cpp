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
              vulkanSwapChain(new VulkanSwapChain(vulkanDevice, vulkanPhysicalDevice, vulkan, window)),
              vertexShader(new VulkanShader(vulkanDevice)),
              fragmentShader(new VulkanShader(vulkanDevice)),
              vulkanRenderPass(new VulkanRenderPass(vulkanSwapChain, vulkanDevice)),
              vulkanGraphicsPipeline(new VulkanGraphicsPipeline(vulkanRenderPass, vulkanSwapChain, vulkanDevice)),
              vulkanCommandPool(new VulkanCommandPool(vulkanPhysicalDevice, vulkanDevice)) {
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
        VD_LOG_INFO("Running...");
        while (!window->shouldClose()) {
            window->pollEvents();
            drawFrame();
        }
        vulkanDevice->waitUntilIdle();
        terminate();
    }

    bool App::initialize() {
        Log::initialize(config.Name, config.LogLevel);
        VD_LOG_INFO("Initializing...");

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
        if (!vulkanCommandPool->initialize()) {
            VD_LOG_ERROR("Could not initialize Vulkan command pool");
            return false;
        }
        vulkanCommandBuffers = vulkanCommandPool->allocateCommandBuffers(MAX_FRAMES_IN_FLIGHT);
        if (vulkanCommandBuffers.empty()) {
            VD_LOG_ERROR("Could not initialize Vulkan command buffers");
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
        if (!initializeRenderingObjects()) {
            VD_LOG_ERROR("Could not initialize Vulkan swap chain");
            return false;
        }
        if (!initializeSyncObjects()) {
            VD_LOG_ERROR("Could not create Vulkan sync objects (semaphores & fences)");
            return false;
        }
        return true;
    }

    bool App::initializeRenderingObjects() {
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
        if (!initializeFramebuffers()) {
            VD_LOG_ERROR("Could not initialize Vulkan framebuffers");
            return false;
        }
        return true;
    }

    bool App::initializeFramebuffers() {
        const std::vector<VkImageView>& swapChainImageViews = vulkanSwapChain->getImageViews();
        for (auto swapChainImageView : swapChainImageViews) {
            VulkanFramebuffer framebuffer(vulkanDevice, vulkanSwapChain, vulkanRenderPass);
            if (!framebuffer.initialize(swapChainImageView)) {
                VD_LOG_ERROR("Could not initialize framebuffers");
                return false;
            }
            framebuffers.push_back(framebuffer);
        }
        VD_LOG_INFO("Created [{}] Vulkan framebuffers", framebuffers.size());
        return true;
    }

    bool App::initializeSyncObjects() {
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        VkAllocationCallbacks* allocationCallbacks = VK_NULL_HANDLE;
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(vulkanDevice->getDevice(), &semaphoreInfo, allocationCallbacks, &imageAvailableSemaphores[i]) != VK_SUCCESS) {
                VD_LOG_ERROR("Could not create 'image available' semaphore for frame [{}]", i);
                return false;
            }
            if (vkCreateSemaphore(vulkanDevice->getDevice(), &semaphoreInfo, allocationCallbacks, &renderFinishedSemaphores[i]) != VK_SUCCESS) {
                VD_LOG_ERROR("Could not create 'render finished' semaphore for frame [{}]", i);
                return false;
            }
            if (vkCreateFence(vulkanDevice->getDevice(), &fenceInfo, allocationCallbacks, &inFlightFences[i]) != VK_SUCCESS) {
                VD_LOG_ERROR("Could not create 'in flight' fence for frame [{}]", i);
                return false;
            }
        }
        VD_LOG_INFO("Created Vulkan sync objects (semaphores & fences)");
        return true;
    }

    void App::terminate() {
        VD_LOG_INFO("Terminating...");
        terminateSyncObjects();
        terminateRenderingObjects();
        fragmentShader->terminate();
        vertexShader->terminate();
        vulkanCommandPool->terminate();
        vulkanDevice->terminate();
        vulkan->terminate();
        window->terminate();
    }

    void App::terminateSyncObjects() const {
        VkAllocationCallbacks* allocationCallbacks = VK_NULL_HANDLE;
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(vulkanDevice->getDevice(), renderFinishedSemaphores[i], allocationCallbacks);
            vkDestroySemaphore(vulkanDevice->getDevice(), imageAvailableSemaphores[i], allocationCallbacks);
            vkDestroyFence(vulkanDevice->getDevice(), inFlightFences[i], allocationCallbacks);
        }
        VD_LOG_INFO("Destroyed Vulkan sync objects (semaphores & fences)");
    }

    void App::terminateRenderingObjects() {
        terminateFramebuffers();
        vulkanGraphicsPipeline->terminate();
        vulkanRenderPass->terminate();
        vulkanSwapChain->terminate();
    }

    void App::terminateFramebuffers() {
        for (VulkanFramebuffer framebuffer : framebuffers) {
            framebuffer.terminate();
        }
        framebuffers.clear();
        VD_LOG_INFO("Destroyed Vulkan framebuffers");
    }

    bool App::recreateRenderingObjects() {
        window->waitUntilNotMinimized();
        vulkanDevice->waitUntilIdle();
        terminateRenderingObjects();
        vulkanPhysicalDevice->updateSwapChainInfo();
        return initializeRenderingObjects();
    }

    void App::drawFrame() {

        /*
         * Preparation
         */

        // Wait until the previous frame has finished
        constexpr uint32_t fenceCount = 1;
        constexpr VkBool32 waitForAllFences = VK_TRUE;
        constexpr uint64_t waitForFenceTimeout = UINT64_MAX;
        VkFence inFlightFence = inFlightFences[currentFrame];
        vkWaitForFences(vulkanDevice->getDevice(), fenceCount, &inFlightFence, waitForAllFences, waitForFenceTimeout);

        // Acquire an image from the swap chain
        uint32_t swapChainImageIndex;
        VkFence acquireNextImageFence = VK_NULL_HANDLE;
        constexpr uint64_t acquireNextImageTimeout = UINT64_MAX;
        VkSemaphore imageAvailableSemaphore = imageAvailableSemaphores[currentFrame];
        VkResult acquireNextImageResult = vkAcquireNextImageKHR(
                vulkanDevice->getDevice(),
                vulkanSwapChain->getSwapChain(),
                acquireNextImageTimeout,
                imageAvailableSemaphore,
                acquireNextImageFence,
                &swapChainImageIndex
        );
        // VK_ERROR_OUT_OF_DATE_KHR: The swap chain has become incompatible with the surface and can no longer be used for rendering. Usually happens after a window resize.
        if (acquireNextImageResult == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateRenderingObjects();
            return;
        }
        // VK_SUBOPTIMAL_KHR: The swap chain can still be used to successfully present to the surface, but the surface properties are no longer matched exactly.
        if (acquireNextImageResult != VK_SUCCESS && acquireNextImageResult != VK_SUBOPTIMAL_KHR) {
            VD_LOG_CRITICAL("Could not acquire swap chain image");
            throw std::runtime_error("Could not acquire swap chain image");
        }

        // After waiting, we need to manually reset the fence to the unsignaled state
        vkResetFences(vulkanDevice->getDevice(), fenceCount, &inFlightFence);

        /*
         * Recording
         */

        VulkanCommandBuffer vulkanCommandBuffer = vulkanCommandBuffers[currentFrame];
        vulkanCommandBuffer.reset();
        vulkanCommandBuffer.begin();

        vulkanRenderPass->begin(vulkanCommandBuffer, framebuffers.at(swapChainImageIndex));
        vulkanGraphicsPipeline->bind(vulkanCommandBuffer);

        constexpr uint32_t vertexCount = 3;
        constexpr uint32_t instanceCount = 1;
        constexpr uint32_t firstVertex = 0;
        constexpr uint32_t firstInstance = 0;
        vkCmdDraw(vulkanCommandBuffer.getCommandBuffer(), vertexCount, instanceCount, firstVertex, firstInstance);

        vulkanRenderPass->end(vulkanCommandBuffer);

        if (!vulkanCommandBuffer.end()) {
            VD_LOG_CRITICAL("Could not end frame");
            throw std::runtime_error("Could not end frame");
        }

        /*
         * Submission
         */

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkCommandBuffer vkCommandBuffer = vulkanCommandBuffer.getCommandBuffer();
        submitInfo.pCommandBuffers = &vkCommandBuffer;
        submitInfo.commandBufferCount = 1;

        // Wait with writing colors to the image until it's available
        VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.waitSemaphoreCount = 1;

        // Which semaphores to signal once the command buffer(s) have finished execution
        VkSemaphore renderFinishedSemaphore = renderFinishedSemaphores[currentFrame];
        VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
        submitInfo.pSignalSemaphores = signalSemaphores;
        submitInfo.signalSemaphoreCount = 1;

        // Submit recorded graphics commands
        constexpr uint32_t submitCount = 1;
        if (vkQueueSubmit(vulkanDevice->getGraphicsQueue(), submitCount, &submitInfo, inFlightFence) != VK_SUCCESS) {
            VD_LOG_CRITICAL("Could not submit to graphics queue");
            throw std::runtime_error("Could not submit to graphics queue");
        }

        /*
         * Presentation
         */

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        // Which semaphores to wait on before presentation can happen
        presentInfo.pWaitSemaphores = signalSemaphores;
        presentInfo.waitSemaphoreCount = 1;

        // Which swap chain to present image to
        VkSwapchainKHR swapChains[] = {vulkanSwapChain->getSwapChain()};
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &swapChainImageIndex;
        presentInfo.swapchainCount = 1;

        // Present image to swap chain
        VkResult presentResult = vkQueuePresentKHR(vulkanDevice->getPresentQueue(), &presentInfo);
        if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR || windowResized) {
            windowResized = false;
            recreateRenderingObjects();
        } else if (presentResult != VK_SUCCESS) {
            VD_LOG_CRITICAL("Could not present image to swap chain");
            throw std::runtime_error("Could not present image to swap chain");
        }

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

}