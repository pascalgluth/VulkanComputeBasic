#include "Application.h"

#include <stdexcept>
#include <iostream>

#include "ShaderLoader.h"

#define CHECK_VK_RESULT(result, str) if ((result) != VK_SUCCESS) throw std::runtime_error((str))

struct Calculation
{
    float f1, f2, res;
};

void Application::Run()
{
    setup();
    runCompute();
}

void Application::setup()
{
    createInstance();
    findPhysicalDevice();
    createDevice();
    createDataBufferAndDescriptorSet();
    createComputePipeline();
    createCommandBuffer();
}

void Application::runCompute()
{
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    CHECK_VK_RESULT(vkBeginCommandBuffer(m_commandBuffer, &beginInfo),
                    "Failed to begin command buffer");
    {
        vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);
        vkCmdBindDescriptorSets(m_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineLayout, 0, 1, &m_descriptorSet, 0, nullptr);

        vkCmdDispatch(m_commandBuffer, 1, 1, 1);
    }
    CHECK_VK_RESULT(vkEndCommandBuffer(m_commandBuffer),
                    "Failed to end command buffer");

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffer;

    VkFence waitFence;
    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = 0;
    CHECK_VK_RESULT(vkCreateFence(m_device, &fenceCreateInfo, nullptr, &waitFence),
                    "Failed to create fence");

    CHECK_VK_RESULT(vkQueueSubmit(m_computeQueue, 1, &submitInfo, waitFence),
                    "Failed to submit queue");

    std::cout << "Waiting for GPU to finish work..." << std::endl;

    CHECK_VK_RESULT(vkWaitForFences(m_device, 1, &waitFence, VK_TRUE, std::numeric_limits<uint64_t>::max()),
                    "Failed to wait for fence");
    std::cout << "Done" << std::endl;
    vkDestroyFence(m_device, waitFence, nullptr);

    void* mappedMem = nullptr;
    vkMapMemory(m_device, m_bufferMemory, 0, sizeof(Calculation), 0, &mappedMem);

    Calculation* calc = (Calculation*)mappedMem;
    if (calc)
    {
        std::cout << "Result: " << calc->res << std::endl;
    }
    else
    {
        std::cerr << "Error reading result" << std::endl;
    }

    vkUnmapMemory(m_device, m_bufferMemory);
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugReportCallbackFn(
        VkDebugReportFlagsEXT                       flags,
        VkDebugReportObjectTypeEXT                  objectType,
        uint64_t                                    object,
        size_t                                      location,
        int32_t                                     messageCode,
        const char*                                 pLayerPrefix,
        const char*                                 pMessage,
        void*                                       pUserData) {

    std::cout << "Vulkan Debug " << pLayerPrefix <<  ": " << pMessage << std::endl;

    return VK_FALSE;
}

void Application::createInstance()
{
    if (this->m_enableValidationLayers && !checkValidationLayerSupport())
        throw std::runtime_error("Validation layers requested, but not available!");

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan Compute Basic Template";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "None";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
#ifdef VK_API_VERSION_1_3
    appInfo.apiVersion = VK_API_VERSION_1_3;
#elif VK_API_VERSION_1_2
    appInfo.apiVersion = VK_API_VERSION_1_2;
#elif VK_API_VERSION_1_1
    appInfo.apiVersion = VK_API_VERSION_1_1;
#elif VK_API_VERSION_1_1
    appInfo.apiVersion = VK_API_VERSION_1_0;
#else
    throw std::runtime_error("No Vulkan SDK found");
#endif

    std::vector<const char *> enabledExtensions;

    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;

    uint32_t extensionCount;

    vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL);
    std::vector<VkExtensionProperties> extensionProperties(extensionCount);
    vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, extensionProperties.data());

    bool foundExtension = false;
    for (VkExtensionProperties prop : extensionProperties) {
        if (strcmp(VK_EXT_DEBUG_REPORT_EXTENSION_NAME, prop.extensionName) == 0) {
            foundExtension = true;
            break;
        }

    }

    if (!foundExtension) {
        throw std::runtime_error("Extension VK_EXT_DEBUG_REPORT_EXTENSION_NAME not supported\n");
    }
    enabledExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

    instanceCreateInfo.enabledExtensionCount = enabledExtensions.size();
    instanceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();

    if (m_enableValidationLayers)
    {
        instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
        instanceCreateInfo.ppEnabledLayerNames = m_validationLayers.data();
    }
    else
    {
        instanceCreateInfo.enabledLayerCount = 0;
        instanceCreateInfo.ppEnabledLayerNames = nullptr;
    }

    VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &m_instance);
    CHECK_VK_RESULT(result, "Failed to create Vulkan Instance");

    if (m_enableValidationLayers) {
        VkDebugReportCallbackCreateInfoEXT createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
        createInfo.pfnCallback = &debugReportCallbackFn;

        // We have to explicitly load this function.
        auto vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(m_instance, "vkCreateDebugReportCallbackEXT");
        if (vkCreateDebugReportCallbackEXT == nullptr) {
            throw std::runtime_error("Could not load vkCreateDebugReportCallbackEXT");
        }

        // Create and register callback.
        CHECK_VK_RESULT(vkCreateDebugReportCallbackEXT(m_instance, &createInfo, NULL, &m_debugReportCallback), "Failed to register debug callback");
    }

}

void Application::findPhysicalDevice()
{
    uint32_t deviceCount;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, NULL);
    if (deviceCount == 0) {
        throw std::runtime_error("Could not find a device with vulkan support");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

    for (VkPhysicalDevice dev : devices)
    {
        m_physicalDevice = dev;
        break;
    }
}

void Application::createDevice()
{
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    uint32_t queueFamilyIndex = getComputeQueueFamilyIndex();
    queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
    queueCreateInfo.queueCount = 1;
    float queuePriorities = 1.0;
    queueCreateInfo.pQueuePriorities = &queuePriorities;

    VkDeviceCreateInfo deviceCreateInfo = {};

    VkPhysicalDeviceFeatures deviceFeatures = {};

    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.enabledLayerCount = m_validationLayers.size();
    deviceCreateInfo.ppEnabledLayerNames = m_validationLayers.data();
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

    CHECK_VK_RESULT(vkCreateDevice(m_physicalDevice, &deviceCreateInfo, NULL, &m_device),
                    "Failed to create device");

    vkGetDeviceQueue(m_device, queueFamilyIndex, 0, &m_computeQueue);
}

void Application::createDataBufferAndDescriptorSet()
{
    // Create buffer

    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = sizeof(Calculation);
    bufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    CHECK_VK_RESULT(vkCreateBuffer(m_device, &bufferCreateInfo, nullptr, &m_buffer),
                    "Failed to create buffer");

    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(m_device, m_buffer, &memReq);

    VkMemoryAllocateInfo allocateInfo = {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = memReq.size;
    allocateInfo.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    CHECK_VK_RESULT(vkAllocateMemory(m_device, &allocateInfo, nullptr, &m_bufferMemory),
                    "Failed to allocate buffer memory");

    CHECK_VK_RESULT(vkBindBufferMemory(m_device, m_buffer, m_bufferMemory, 0),
                    "Failed to bind buffer memory");

    Calculation calc;
    calc.f1 = 3.14f;
    calc.f2 = 8.43f;
    calc.res = 0.f;

    void* mappedMem = nullptr;
    vkMapMemory(m_device, m_bufferMemory, 0, sizeof(Calculation), 0, &mappedMem);
    memcpy(mappedMem, &calc, sizeof(calc));
    vkUnmapMemory(m_device, m_bufferMemory);

    // Create descriptor set layout

    VkDescriptorSetLayoutBinding descriptorSetLayoutBinding = {};
    descriptorSetLayoutBinding.binding = 0;
    descriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorSetLayoutBinding.descriptorCount = 1;
    descriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCreateInfo.bindingCount = 1;
    layoutCreateInfo.pBindings = &descriptorSetLayoutBinding;

    CHECK_VK_RESULT(vkCreateDescriptorSetLayout(m_device, &layoutCreateInfo, nullptr, &m_descriptorSetLayout),
                    "Failed to create Descriptor set layout");

    // Create descriptor pool

    VkDescriptorPoolSize descriptorPoolSize = {};
    descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorPoolSize.descriptorCount = 1;

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
    descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolCreateInfo.maxSets = 1;
    descriptorPoolCreateInfo.poolSizeCount = 1;
    descriptorPoolCreateInfo.pPoolSizes = &descriptorPoolSize;

    CHECK_VK_RESULT(vkCreateDescriptorPool(m_device, &descriptorPoolCreateInfo, nullptr, &m_descriptorPool),
                    "Failed to create descriptor pool");

    // Allocate descriptor set

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_descriptorSetLayout;

    CHECK_VK_RESULT(vkAllocateDescriptorSets(m_device, &allocInfo, &m_descriptorSet),
                    "Failed to allocate descriptor set");

    // Update descriptor set

    VkDescriptorBufferInfo descriptorBufferInfo = {};
    descriptorBufferInfo.buffer = m_buffer;
    descriptorBufferInfo.offset = 0;
    descriptorBufferInfo.range = sizeof(Calculation);

    VkWriteDescriptorSet writeDescriptorSet = {};
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.dstSet = m_descriptorSet;
    writeDescriptorSet.dstBinding = 0;
    writeDescriptorSet.descriptorCount = 1;
    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet.pBufferInfo = &descriptorBufferInfo;

    vkUpdateDescriptorSets(m_device, 1, &writeDescriptorSet, 0, NULL);
}

void Application::createComputePipeline()
{
    VkPipelineShaderStageCreateInfo compShader = ShaderLoader::compileAndLoadShader(m_device, "../Shaders/compShader.glsl", "compShader", VK_SHADER_STAGE_COMPUTE_BIT);

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &m_descriptorSetLayout;
    CHECK_VK_RESULT(vkCreatePipelineLayout(m_device, &pipelineLayoutCreateInfo, NULL, &m_pipelineLayout),
                    "Failed to create compute pipeline layout");

    VkComputePipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stage = compShader;
    pipelineCreateInfo.layout = m_pipelineLayout;

    CHECK_VK_RESULT(vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, NULL, &m_pipeline),
                    "Failed to create compute pipeline");
}

void Application::createCommandBuffer()
{
    VkCommandPoolCreateInfo commandPoolCreateInfo = {};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.flags = 0;
    commandPoolCreateInfo.queueFamilyIndex = getComputeQueueFamilyIndex();
    CHECK_VK_RESULT(vkCreateCommandPool(m_device, &commandPoolCreateInfo, NULL, &m_commandPool),
                    "Failed to create command pool");

    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = m_commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = 1;
    CHECK_VK_RESULT(vkAllocateCommandBuffers(m_device, &commandBufferAllocateInfo, &m_commandBuffer),
                    "Failed to allocate command buffer");
}

bool Application::checkValidationLayerSupport()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : m_validationLayers)
    {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

uint32_t Application::getComputeQueueFamilyIndex()
{
    uint32_t queueFamilyCount;

    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, NULL);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, queueFamilies.data());

    uint32_t i = 0;
    for (; i < queueFamilies.size(); ++i) {
        VkQueueFamilyProperties props = queueFamilies[i];

        if (props.queueCount > 0 && (props.queueFlags & VK_QUEUE_COMPUTE_BIT)) {
            break;
        }
    }

    if (i == queueFamilies.size()) {
        throw std::runtime_error("Could not find a queue family that supports operations");
    }

    return i;
}

uint32_t Application::findMemoryType(uint32_t memoryTypeBits, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memoryProperties;

    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memoryProperties);

    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
        if ((memoryTypeBits & (1 << i)) &&
            ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties))
            return i;
    }
    return -1;
}




