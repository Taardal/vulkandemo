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
              vertexShader(new VulkanShader(vulkanDevice)),
              fragmentShader(new VulkanShader(vulkanDevice)),
              vulkanSwapChain(new VulkanSwapChain(vulkanDevice, vulkanPhysicalDevice, vulkan, window)),
              vulkanRenderPass(new VulkanRenderPass(vulkanSwapChain, vulkanDevice)),
              vulkanGraphicsPipeline(new VulkanGraphicsPipeline(vulkanRenderPass, vulkanSwapChain, vulkanDevice)) {
    }

    App::~App() {
        delete vulkanGraphicsPipeline;
        delete vulkanRenderPass;
        delete vulkanSwapChain;
        delete fragmentShader;
        delete vertexShader;
        delete vulkanCommandPool;
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
            VD_LOG_ERROR("Could not initialize Vulkan rendering objects (swap chain & dependents)");
            return false;
        }
        if (!initializeVertexBuffer()) {
            VD_LOG_ERROR("Could not initialize Vulkan vertex buffer");
            return false;
        }
        if (!initializeIndexBuffer()) {
            VD_LOG_ERROR("Could not initialize Vulkan index buffer");
            return false;
        }
        if (!initializeSyncObjects()) {
            VD_LOG_ERROR("Could not create Vulkan sync objects (semaphores & fences)");
            return false;
        }
        return true;
    }

    bool App::initializeIndexBuffer() {
        VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        constexpr VkBufferUsageFlags stagingBufferUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        constexpr VkMemoryPropertyFlags stagingBufferMemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        createBuffer(bufferSize, stagingBufferUsage, stagingBufferMemoryProperties, stagingBuffer, stagingBufferMemory);

        void* memory;
        constexpr VkDeviceSize stagingBufferMemoryOffset = 0;
        constexpr VkMemoryMapFlags stagingBufferMemoryMapFlags = 0;
        vkMapMemory(vulkanDevice->getDevice(), stagingBufferMemory, stagingBufferMemoryOffset, bufferSize, stagingBufferMemoryMapFlags, &memory);
        memcpy(memory, indices.data(), (size_t) bufferSize);
        vkUnmapMemory(vulkanDevice->getDevice(), stagingBufferMemory);

        constexpr VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        constexpr VkMemoryPropertyFlags bufferMemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        if (!createBuffer(bufferSize, bufferUsage, bufferMemoryProperties, indexBuffer, indexBufferDeviceMemory)) {
            VD_LOG_ERROR("Could not create index buffer");
            return false;
        }

        copyBuffer(stagingBuffer, indexBuffer, bufferSize);

        VkAllocationCallbacks* allocator = VK_NULL_HANDLE;
        vkDestroyBuffer(vulkanDevice->getDevice(), stagingBuffer, allocator);
        vkFreeMemory(vulkanDevice->getDevice(), stagingBufferMemory, allocator);

        return true;
    }

    bool App::initializeVertexBuffer() {
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

        VulkanBuffer::Config stagingBufferConfig{};
        stagingBufferConfig.Size = bufferSize;
        stagingBufferConfig.Usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        stagingBufferConfig.MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        VulkanBuffer stagingBuffer(vulkanPhysicalDevice, vulkanDevice);
        if (!stagingBuffer.initialize(stagingBufferConfig)) {
            VD_LOG_ERROR("Could not create staging vertex buffer");
            return false;
        }

        void* memory;
        constexpr VkDeviceSize stagingBufferMemoryOffset = 0;
        constexpr VkMemoryMapFlags stagingBufferMemoryMapFlags = 0;
        vkMapMemory(vulkanDevice->getDevice(), stagingBuffer.getDeviceMemory(), stagingBufferMemoryOffset, stagingBufferConfig.Size, stagingBufferMemoryMapFlags, &memory);
        memcpy(memory, vertices.data(), (size_t) stagingBufferConfig.Size);
        vkUnmapMemory(vulkanDevice->getDevice(), stagingBuffer.getDeviceMemory());

        VulkanBuffer::Config vertexBufferConfig{};
        stagingBufferConfig.Size = bufferSize;
        stagingBufferConfig.Usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        stagingBufferConfig.MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        if (!vertexBuffer->initialize(vertexBufferConfig)) {
            VD_LOG_ERROR("Could not create vertex buffer");
            return false;
        }

        copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

        stagingBuffer.terminate();
        return true;
    }

    void App::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const {
        VulkanCommandBuffer commandBuffer = vulkanCommandPool->allocateCommandBuffers(1)[0];
        commandBuffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        constexpr uint32_t regionCount = 1;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, regionCount, &copyRegion);

        if (!commandBuffer.end()) {
            VD_LOG_ERROR("Could not end Vulkan command buffer");
            return false;
        }

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;

        VkCommandBuffer vkCommandBuffer = commandBuffer.getCommandBuffer();
        submitInfo.pCommandBuffers = &vkCommandBuffer;

        constexpr uint32_t submitCount = 1;
        VkFence fence = VK_NULL_HANDLE;
        vkQueueSubmit(vulkanDevice->getGraphicsQueue(), submitCount, &submitInfo, fence);
        vkQueueWaitIdle(vulkanDevice->getGraphicsQueue());

        vulkanCommandPool->freeCommandBuffer(commandBuffer);
        return true;
    }

    bool App::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties, VkBuffer& buffer, VkDeviceMemory& bufferDeviceMemory) {
        VkAllocationCallbacks* allocator = VK_NULL_HANDLE;

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(vulkanDevice->getDevice(), &bufferInfo, allocator, &buffer) != VK_SUCCESS) {
            VD_LOG_ERROR("Could not create Vulkan vertex buffer");
            return false;
        }

        VkMemoryRequirements memoryRequirements;
        vkGetBufferMemoryRequirements(vulkanDevice->getDevice(), buffer, &memoryRequirements);

        VkMemoryAllocateInfo memoryAllocateInfo{};
        memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memoryAllocateInfo.allocationSize = memoryRequirements.size;
        memoryAllocateInfo.memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, memoryProperties);

        if (vkAllocateMemory(vulkanDevice->getDevice(), &memoryAllocateInfo, allocator, &bufferDeviceMemory) != VK_SUCCESS) {
            VD_LOG_ERROR("Could not allocate Vulkan vertex buffer memory");
            return false;
        }

        constexpr VkDeviceSize memoryOffset = 0;
        vkBindBufferMemory(vulkanDevice->getDevice(), buffer, bufferDeviceMemory, memoryOffset);

        return true;
    }

    uint32_t App::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags propertyFlags) const {
        VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
        vkGetPhysicalDeviceMemoryProperties(vulkanPhysicalDevice->getPhysicalDevice(), &physicalDeviceMemoryProperties);

        for (uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags) {
                return i;
            }
        }
        VD_LOG_ERROR("Could not find memory type [{}]", typeFilter);
        return -1;
    }

    void App::terminateIndexBuffer() {
        VkAllocationCallbacks* allocator = VK_NULL_HANDLE;
        vkDestroyBuffer(vulkanDevice->getDevice(), indexBuffer, allocator);
        vkFreeMemory(vulkanDevice->getDevice(), indexBufferDeviceMemory, allocator);
    }

    void App::terminateVertexBuffer() {
        VkAllocationCallbacks* allocator = VK_NULL_HANDLE;
        vkDestroyBuffer(vulkanDevice->getDevice(), vertexBuffer, allocator);
        vkFreeMemory(vulkanDevice->getDevice(), vertexBufferDeviceMemory, allocator);
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
        indexBuffer->terminate();
        vertexBuffer->terminate();
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
         * Command recording
         */

        const VulkanCommandBuffer& vulkanCommandBuffer = vulkanCommandBuffers[currentFrame];
        vulkanCommandBuffer.reset();
        vulkanCommandBuffer.begin();

        vulkanRenderPass->begin(vulkanCommandBuffer, framebuffers.at(swapChainImageIndex));
        vulkanGraphicsPipeline->bind(vulkanCommandBuffer);

        VkBuffer vertexBuffers[] = {vertexBuffer};
        VkDeviceSize vertexBufferOffsets[] = {0};
        constexpr uint32_t firstBinding = 0;
        constexpr uint32_t bindingCount = 1;
        vkCmdBindVertexBuffers(vulkanCommandBuffer.getCommandBuffer(), firstBinding, bindingCount, vertexBuffers, vertexBufferOffsets);

        constexpr VkDeviceSize indexBufferOffset = 0;
        constexpr VkIndexType indexType = VK_INDEX_TYPE_UINT16;
        vkCmdBindIndexBuffer(vulkanCommandBuffer.getCommandBuffer(), indexBuffer, indexBufferOffset, indexType);

        constexpr uint32_t instanceCount = 1;
        constexpr uint32_t firstVertex = 0;
        constexpr int32_t vertexOffset = 0;
        constexpr uint32_t firstInstance = 0;
        vkCmdDrawIndexed(vulkanCommandBuffer.getCommandBuffer(), (uint32_t) indices.size(), instanceCount, firstVertex, vertexOffset, firstInstance);

        vulkanRenderPass->end(vulkanCommandBuffer);

        if (!vulkanCommandBuffer.end()) {
            VD_LOG_CRITICAL("Could not end frame");
            throw std::runtime_error("Could not end frame");
        }

        /*
         * Command submission
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
         * Image presentation
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