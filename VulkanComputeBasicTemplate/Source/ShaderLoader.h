#pragma once

#include <vulkan/vulkan.h>
#include <string>

class ShaderLoader
{
public:
    static VkPipelineShaderStageCreateInfo loadShader(VkDevice device, const std::string &file, VkShaderStageFlagBits stage);
    static VkPipelineShaderStageCreateInfo compileAndLoadShader(VkDevice device, const std::string& file, const std::string& sourceName, VkShaderStageFlagBits stage);

};
