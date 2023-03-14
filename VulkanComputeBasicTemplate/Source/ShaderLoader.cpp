#include "ShaderLoader.h"

#include <shaderc/shaderc.hpp>
#include <vector>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <sstream>

#define CHECK_VK_RESULT(result, str) if ((result) != VK_SUCCESS) throw std::runtime_error((str))

std::vector<VkShaderModule> shaderModuleCache = {};

std::vector<char> readFile(const std::string& fileName)
{
    // Open File
    std::ifstream file(fileName, std::ios::binary | std::ios::ate);
    if (!file.is_open())
        throw std::runtime_error("Failed to read file: " + fileName);

    // Create vector with size
    size_t fileSize = (size_t)file.tellg();
    std::vector<char> fileBuffer(fileSize);

    // Set read position to start of file
    file.seekg(0);

    // Read all file data into the buffer
    file.read(fileBuffer.data(), fileSize);

    // Close file
    file.close();

    return fileBuffer;
}

std::string readFileText(const std::string& fullpath)
{
    std::ifstream file(fullpath);
    std::stringstream stream;
    stream << file.rdbuf();
    file.close();
    return stream.str();
}

VkPipelineShaderStageCreateInfo ShaderLoader::loadShader(VkDevice device, const std::string &file, VkShaderStageFlagBits stage)
{
    auto shaderSpv = readFile(file);

    VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.codeSize = shaderSpv.size();
    shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(shaderSpv.data());

    VkShaderModule shaderModule;
    VkResult result = vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &shaderModule);
    CHECK_VK_RESULT(result, "Failed to create Shader Module");

    shaderModuleCache.push_back(shaderModule);

    VkPipelineShaderStageCreateInfo shaderStageCreateInfo = {};
    shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCreateInfo.stage = stage;
    shaderStageCreateInfo.module = shaderModule;
    shaderStageCreateInfo.pName = "main";

    return shaderStageCreateInfo;
}

VkPipelineShaderStageCreateInfo ShaderLoader::compileAndLoadShader(VkDevice device, const std::string &file, const std::string &sourceName,
                                   VkShaderStageFlagBits stage)
{
    std::string shaderSource = readFileText(file);

    shaderc_shader_kind shaderKind;
    switch (stage)
    {
        case VK_SHADER_STAGE_VERTEX_BIT:
            shaderKind = shaderc_glsl_default_vertex_shader;
            break;
        case VK_SHADER_STAGE_FRAGMENT_BIT:
            shaderKind = shaderc_glsl_default_fragment_shader;
            break;
        case VK_SHADER_STAGE_COMPUTE_BIT:
            shaderKind = shaderc_glsl_default_compute_shader;
            break;
    }

    shaderc::Compiler compiler;
    shaderc::CompileOptions options;

    shaderc::SpvCompilationResult shaderRes = compiler.CompileGlslToSpv(shaderSource, shaderKind, sourceName.c_str(), options);
    if (shaderRes.GetCompilationStatus() != shaderc_compilation_status_success)
        throw std::runtime_error(shaderRes.GetErrorMessage());

    std::vector<uint32_t> shaderSpv = { shaderRes.cbegin(), shaderRes.cend() };

    VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.codeSize = sizeof(uint32_t) * shaderSpv.size();
    shaderModuleCreateInfo.pCode = shaderSpv.data();

    VkShaderModule shaderModule;
    VkResult result = vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &shaderModule);
    CHECK_VK_RESULT(result, "Failed to create Shader Module");

    shaderModuleCache.push_back(shaderModule);

    VkPipelineShaderStageCreateInfo shaderStageCreateInfo = {};
    shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCreateInfo.stage = stage;
    shaderStageCreateInfo.module = shaderModule;
    shaderStageCreateInfo.pName = "main";

    return std::move(shaderStageCreateInfo);
}
