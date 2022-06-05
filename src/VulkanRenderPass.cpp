#include "VulkanRenderPass.h"
#include "VulkanFramebuffer.h"
#include "Log.h"

namespace Vulkandemo {

    const VkAllocationCallbacks* VulkanRenderPass::ALLOCATOR = VK_NULL_HANDLE;

    VulkanRenderPass::VulkanRenderPass(VulkanSwapChain* vulkanSwapChain, VulkanDevice* vulkanDevice) : vulkanSwapChain(vulkanSwapChain), vulkanDevice(vulkanDevice) {
    }

    const VkRenderPass VulkanRenderPass::getRenderPass() const {
        return renderPass;
    }

    bool VulkanRenderPass::initialize() {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = vulkanSwapChain->getSurfaceFormat().format;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(vulkanDevice->getDevice(), &renderPassInfo, ALLOCATOR, &renderPass) != VK_SUCCESS) {
            VD_LOG_ERROR("Could not create Vulkan render pass");
            return false;
        }
        VD_LOG_INFO("Created Vulkan render pass");
        return true;
    }

    void VulkanRenderPass::terminate() {
        vkDestroyRenderPass(vulkanDevice->getDevice(), renderPass, ALLOCATOR);
        VD_LOG_INFO("Destroyed Vulkan render pass");
    }

    void VulkanRenderPass::begin(const VulkanCommandBuffer& vulkanCommandBuffer, const VulkanFramebuffer& vulkanFramebuffer) const {
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = vulkanFramebuffer.getFramebuffer();
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = vulkanSwapChain->getExtent();

        VkClearColorValue clearColorValue = {
                {0.0f, 0.0f, 0.0f, 1.0f}
        };
        VkClearValue clearColor = {
                clearColorValue
        };
        renderPassInfo.pClearValues = &clearColor;
        renderPassInfo.clearValueCount = 1;

        vkCmdBeginRenderPass(vulkanCommandBuffer.getCommandBuffer(), &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

    void VulkanRenderPass::end(const VulkanCommandBuffer& vulkanCommandBuffer) const {
        vkCmdEndRenderPass(vulkanCommandBuffer.getCommandBuffer());
    }

}
