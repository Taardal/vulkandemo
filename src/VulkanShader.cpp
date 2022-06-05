#include "VulkanShader.h"
#include "Log.h"

namespace Vulkandemo {

    const VkAllocationCallbacks* VulkanShader::ALLOCATOR = VK_NULL_HANDLE;

    VulkanShader::VulkanShader(VulkanDevice* vulkanDevice) : vulkanDevice(vulkanDevice) {
    }

    const VkShaderModule VulkanShader::getShaderModule() const {
        return shaderModule;
    }

    bool VulkanShader::initialize(const std::vector<char>& bytes) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = bytes.size();
        createInfo.pCode = (const uint32_t*) bytes.data();

        if (vkCreateShaderModule(vulkanDevice->getDevice(), &createInfo, ALLOCATOR, &shaderModule) != VK_SUCCESS) {
            VD_LOG_ERROR("Could not create Vulkan shader module");
            return false;
        }

        VD_LOG_INFO("Created Vulkan shader module");
        return true;
    }

    void VulkanShader::terminate() {
        vkDestroyShaderModule(vulkanDevice->getDevice(), shaderModule, ALLOCATOR);
        VD_LOG_INFO("Destroyed Vulkan shader module");
    }


}