#include "VulkanSwapChain.h"
#include "Log.h"

namespace Vulkandemo {

    const VkAllocationCallbacks* VulkanSwapChain::ALLOCATOR = VK_NULL_HANDLE;

    VulkanSwapChain::VulkanSwapChain(VulkanDevice* vulkanDevice, VulkanPhysicalDevice* vulkanPhysicalDevice, Vulkan* vulkan, Window* window)
            : vulkanDevice(vulkanDevice), vulkanPhysicalDevice(vulkanPhysicalDevice), vulkan(vulkan), window(window) {
    }

    const VkSwapchainKHR VulkanSwapChain::getSwapChain() const {
        return swapChain;
    }

    const VkSurfaceFormatKHR& VulkanSwapChain::getSurfaceFormat() const {
        return surfaceFormat;
    }

    const VkExtent2D& VulkanSwapChain::getExtent() const {
        return extent;
    }

    const std::vector<VkImageView>& VulkanSwapChain::getImageViews() const {
        return imageViews;
    }

    bool VulkanSwapChain::initialize() {
        const SwapChainInfo& swapChainInfo = vulkanPhysicalDevice->getSwapChainInfo();

        surfaceFormat = chooseSurfaceFormat(swapChainInfo.SurfaceFormats);
        presentMode = choosePresentMode(swapChainInfo.PresentModes);
        extent = chooseExtent(swapChainInfo.SurfaceCapabilities);

        uint32_t imageCount = getImageCount(swapChainInfo.SurfaceCapabilities);

        if (!createSwapChain(swapChainInfo.SurfaceCapabilities, imageCount)) {
            VD_LOG_ERROR("Could not create Vulkan swap chain");
            return false;
        }
        VD_LOG_INFO("Created Vulkan swap chain");

        if (!findSwapChainImages(imageCount)) {
            VD_LOG_ERROR("Could not find any Vulkan swap chain images");
            return false;
        }
        VD_LOG_INFO("Initialized [{}] Vulkan swap chain images", images.size());

        if (!createSwapChainImageViews()) {
            VD_LOG_ERROR("Could not create Vulkan swap chain image views");
            return false;
        }
        VD_LOG_INFO("Created [{}] Vulkan swap chain image views", imageViews.size());

        return true;
    }

    void VulkanSwapChain::terminate() {
        for (VkImageView imageView : imageViews) {
            vkDestroyImageView(vulkanDevice->getDevice(), imageView, ALLOCATOR);
        }
        imageViews.clear();
        VD_LOG_INFO("Destroyed Vulkan swap chain image views");
        vkDestroySwapchainKHR(vulkanDevice->getDevice(), swapChain, ALLOCATOR);
        VD_LOG_INFO("Destroyed Vulkan swap chain");
    }

    VkSurfaceFormatKHR VulkanSwapChain::chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const {
        for (const auto& availableFormat: availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }
        VD_LOG_WARN("Could not find target format so defaulting to first available");
        return availableFormats[0];
    }

    VkPresentModeKHR VulkanSwapChain::choosePresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) const {
        VkPresentModeKHR targetPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
        for (const auto& availablePresentMode: availablePresentModes) {
            if (availablePresentMode == targetPresentMode) {
                return availablePresentMode;
            }
        }
        VkPresentModeKHR defaultPresentMode = VK_PRESENT_MODE_FIFO_KHR;
        VD_LOG_WARN("Could not find [{0}] present mode so defaulting to [{1}]", getPresentationModeAsString(targetPresentMode), getPresentationModeAsString(defaultPresentMode));
        return defaultPresentMode;
    }

    VkExtent2D VulkanSwapChain::chooseExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities) const {
        bool extentSizeCanDifferFromWindowResolution = surfaceCapabilities.currentExtent.width == std::numeric_limits<uint32_t>::max();
        if (!extentSizeCanDifferFromWindowResolution) {
            VD_LOG_DEBUG("Extent should match window resolution so using the surface capabilities extent");
            return surfaceCapabilities.currentExtent;
        }
        VD_LOG_DEBUG("Extent can differ from window resolution so picking the resolution that best matches the window within the minImageExtent and maxImageExtent bounds");
        Size windowSizeInPixels = window->getSizeInPixels();
        VkExtent2D extent = {
                (uint32_t) windowSizeInPixels.Width,
                (uint32_t) windowSizeInPixels.Height
        };
        extent.width = std::clamp(extent.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
        extent.height = std::clamp(extent.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
        return extent;
    }

    uint32_t VulkanSwapChain::getImageCount(const VkSurfaceCapabilitiesKHR& surfaceCapabilities) const {
        uint32_t minImageCount = surfaceCapabilities.minImageCount;
        uint32_t maxImageCount = surfaceCapabilities.maxImageCount;
        uint32_t imageCount = minImageCount + 1;
        if (maxImageCount > 0 && imageCount > maxImageCount) {
            imageCount = maxImageCount;
        }
        return imageCount;
    }

    bool VulkanSwapChain::createSwapChain(const VkSurfaceCapabilitiesKHR& surfaceCapabilities, uint32_t imageCount) {
        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = vulkan->getSurface();
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        const QueueFamilyIndices& queueFamilyIndices = vulkanPhysicalDevice->getQueueFamilyIndices();
        if (queueFamilyIndices.GraphicsFamily != queueFamilyIndices.PresentationFamily) {
            uint32_t queueFamilyIndexValues[] = {
                    queueFamilyIndices.GraphicsFamily.value(),
                    queueFamilyIndices.PresentationFamily.value()
            };
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.pQueueFamilyIndices = queueFamilyIndexValues;
            createInfo.queueFamilyIndexCount = 2;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.pQueueFamilyIndices = nullptr;
            createInfo.queueFamilyIndexCount = 0;
        }

        createInfo.preTransform = surfaceCapabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        return vkCreateSwapchainKHR(vulkanDevice->getDevice(), &createInfo, ALLOCATOR, &swapChain) == VK_SUCCESS;
    }

    bool VulkanSwapChain::findSwapChainImages(uint32_t imageCount) {
        vkGetSwapchainImagesKHR(vulkanDevice->getDevice(), swapChain, &imageCount, nullptr);
        images.resize(imageCount);
        vkGetSwapchainImagesKHR(vulkanDevice->getDevice(), swapChain, &imageCount, images.data());
        return !images.empty();
    }

    bool VulkanSwapChain::createSwapChainImageViews() {
        imageViews.resize(images.size());
        for (size_t i = 0; i < images.size(); i++) {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = images[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = surfaceFormat.format;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;
            if (vkCreateImageView(vulkanDevice->getDevice(), &createInfo, ALLOCATOR, &imageViews[i]) != VK_SUCCESS) {
                return false;
            }
        }
        return !imageViews.empty();
    }

    std::string VulkanSwapChain::getPresentationModeAsString(VkPresentModeKHR presentMode) const {
        switch (presentMode) {
            case VK_PRESENT_MODE_IMMEDIATE_KHR:
                return "VK_PRESENT_MODE_IMMEDIATE_KHR";
            case VK_PRESENT_MODE_MAILBOX_KHR:
                return "VK_PRESENT_MODE_MAILBOX_KHR";
            case VK_PRESENT_MODE_FIFO_KHR:
                return "VK_PRESENT_MODE_FIFO_KHR";
            case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
                return "VK_PRESENT_MODE_FIFO_RELAXED_KHR";
            case VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR:
                return "VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR";
            case VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR:
                return "VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR";
            case VK_PRESENT_MODE_MAX_ENUM_KHR:
                return "VK_PRESENT_MODE_MAX_ENUM_KHR";
            default:
                return "";
        }
    }

}
