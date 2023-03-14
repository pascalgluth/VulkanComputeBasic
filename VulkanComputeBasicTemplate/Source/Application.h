#pragma once

#define VK_DEBUG

#include <vulkan/vulkan.h>
#include <vector>

class Application
{
public:
    void Run();

private:
    void setup();
    void runCompute();

    void createInstance();
    void findPhysicalDevice();
    void createDevice();
    void createDataBufferAndDescriptorSet();
    void createComputePipeline();
    void createCommandBuffer();
    uint32_t getComputeQueueFamilyIndex();
    uint32_t findMemoryType(uint32_t memoryTypeBits, VkMemoryPropertyFlags properties);

    VkInstance m_instance;
    VkDebugReportCallbackEXT m_debugReportCallback;
    VkPhysicalDevice m_physicalDevice;
    VkDevice m_device;

    VkPipeline m_pipeline;
    VkPipelineLayout m_pipelineLayout;

    VkCommandPool m_commandPool;
    VkCommandBuffer m_commandBuffer;

    VkQueue m_computeQueue;

    VkBuffer m_buffer;
    VkDeviceMemory m_bufferMemory;

    VkDescriptorPool m_descriptorPool;
    VkDescriptorSet m_descriptorSet;
    VkDescriptorSetLayout m_descriptorSetLayout;



#ifdef VK_DEBUG
    const bool m_enableValidationLayers = true;
#else
    const bool m_enableValidationLayers = false;
#endif
    const std::vector<const char*> m_validationLayers = {
            "VK_LAYER_KHRONOS_validation"
    };
    bool checkValidationLayerSupport();

    const std::vector<const char*> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_KHR_MAINTENANCE1_EXTENSION_NAME,
    };

};
