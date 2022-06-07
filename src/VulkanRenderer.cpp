#include "Log.h"
#include "VulkanRenderer.h"

namespace Vulkandemo {

    const uint32_t VulkanRenderer::MAX_FRAMES_IN_FLIGHT = 2;

    bool VulkanRenderer::initialize() {
        vulkanCommandBuffers = vulkanCommandPool->allocateCommandBuffers(MAX_FRAMES_IN_FLIGHT);
        if (vulkanCommandBuffers.empty()) {
            VD_LOG_ERROR("Could not create [{}] Vulkan command buffers", MAX_FRAMES_IN_FLIGHT);
            return false;
        }
        if (!initializeFramebuffers()) {
            VD_LOG_ERROR("Could not create Vulkan framebuffers");
            return false;
        }
        if (!initializeSyncObjects()) {
            VD_LOG_ERROR("Could not create Vulkan sync objects (semaphores & fences)");
            return false;
        }
        return true;
    }

    bool VulkanRenderer::initializeFramebuffers() {
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

    bool VulkanRenderer::initializeSyncObjects() {
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
        return true;
    }

    void VulkanRenderer::terminate() {
        terminateSyncObjects();
        terminateFramebuffers();
    }

    void VulkanRenderer::terminateSyncObjects() {
        VkAllocationCallbacks* allocationCallbacks = VK_NULL_HANDLE;
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(vulkanDevice->getDevice(), renderFinishedSemaphores[i], allocationCallbacks);
            vkDestroySemaphore(vulkanDevice->getDevice(), imageAvailableSemaphores[i], allocationCallbacks);
            vkDestroyFence(vulkanDevice->getDevice(), inFlightFences[i], allocationCallbacks);
        }
        VD_LOG_INFO("Destroyed Vulkan sync objects (semaphores & fences)");
    }

    void VulkanRenderer::terminateFramebuffers() {
        int framebufferCount = (int) framebuffers.size();
        for (VulkanFramebuffer framebuffer : framebuffers) {
            framebuffer.terminate();
        }
        framebuffers.clear();
        VD_LOG_INFO("Destroyed [{}] Vulkan framebuffers", framebufferCount);
    }

    void VulkanRenderer::drawFrame() {
        constexpr uint32_t fenceCount = 1;
        constexpr VkBool32 waitForAllFences = VK_TRUE;
        constexpr uint64_t waitForFenceTimeout = UINT64_MAX;
        VkFence inFlightFence = inFlightFences[currentFrame];
        vkWaitForFences(vulkanDevice->getDevice(), fenceCount, &inFlightFence, waitForAllFences, waitForFenceTimeout);

        uint32_t swapChainImageIndex;
        VkFence acquireNextImageFence = VK_NULL_HANDLE;
        constexpr uint64_t acquireNextImageTimeout = UINT64_MAX;
        VkSemaphore imageAvailableSemaphore = imageAvailableSemaphores[currentFrame];

        VkResult acquireNextImageResult = vkAcquireNextImageKHR(vulkanDevice->getDevice(), vulkanSwapChain->getSwapChain(), acquireNextImageTimeout, imageAvailableSemaphore, acquireNextImageFence, &swapChainImageIndex);
        if (acquireNextImageResult == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            return;
        }
        if (acquireNextImageResult != VK_SUCCESS && acquireNextImageResult != VK_SUBOPTIMAL_KHR) {
            VD_LOG_CRITICAL("Could not acquire swap chain image");
            throw std::runtime_error("Could not acquire swap chain image");
        }

        vkResetFences(vulkanDevice->getDevice(), fenceCount, &inFlightFence);

        const VulkanCommandBuffer& vulkanCommandBuffer = vulkanCommandBuffers[currentFrame];
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

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        VkCommandBuffer commandBuffer = vulkanCommandBuffer.getCommandBuffer();
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        VkSemaphore renderFinishedSemaphore = renderFinishedSemaphores[currentFrame];
        VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        constexpr uint32_t submitCount = 1;
        if (vkQueueSubmit(vulkanDevice->getGraphicsQueue(), submitCount, &submitInfo, inFlightFence) != VK_SUCCESS) {
            VD_LOG_CRITICAL("Could not submit to graphics queue");
            throw std::runtime_error("Could not submit to graphics queue");
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = {vulkanSwapChain->getSwapChain()};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &swapChainImageIndex;

        VkResult presentResult = vkQueuePresentKHR(vulkanDevice->getPresentQueue(), &presentInfo);
        if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR || windowResized) {
            windowResized = false;
            recreateSwapChain();
        } else if (presentResult != VK_SUCCESS) {
            VD_LOG_CRITICAL("Could not present swap chain image");
            throw std::runtime_error("Could not present swap chain image");
        }

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }


}