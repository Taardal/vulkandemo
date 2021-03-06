#pragma once

#include "VulkanDevice.h"
#include "VulkanPhysicalDevice.h"
#include "Vulkan.h"
#include "Window.h"

#include <vulkan/vulkan.h>
#include <vector>

namespace Vulkandemo {

    class VulkanSwapChain {
    private:
        static const VkAllocationCallbacks* ALLOCATOR;

    private:
        VulkanDevice* vulkanDevice;
        VulkanPhysicalDevice* vulkanPhysicalDevice;
        Vulkan* vulkan;
        Window* window;
        VkSurfaceFormatKHR surfaceFormat{};
        VkPresentModeKHR presentMode{};
        VkExtent2D extent{};
        VkSwapchainKHR swapChain = VK_NULL_HANDLE;
        std::vector<VkImage> images;
        std::vector<VkImageView> imageViews;

    public:
        VulkanSwapChain(VulkanDevice* vulkanDevice, VulkanPhysicalDevice* vulkanPhysicalDevice, Vulkan* vulkan, Window* window);

        const VkSwapchainKHR getSwapChain() const;

        const VkSurfaceFormatKHR& getSurfaceFormat() const;

        const VkExtent2D& getExtent() const;

        const std::vector<VkImageView>& getImageViews() const;

        bool initialize();

        void terminate();

    private:
        VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const;

        VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) const;

        VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities) const;

        uint32_t getImageCount(const VkSurfaceCapabilitiesKHR& surfaceCapabilities) const;

        bool createSwapChain(const VkSurfaceCapabilitiesKHR& surfaceCapabilities, uint32_t imageCount);

        bool findSwapChainImages(uint32_t imageCount);

        bool createSwapChainImageViews();

        std::string getPresentationModeAsString(VkPresentModeKHR presentMode) const;
    };

}


