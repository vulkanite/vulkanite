// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#include "vulkanite/DriverInstance.h"
#include "vulkanite/LogicalDevice.h"
#include "vulkanite/Context.h"

namespace vkn {
    
  void Allocator::create(LogicalDevice *device)
  {
    VmaVulkanFunctions vulkanFunctions = {};
    vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
    vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

    PhysicalDevice::SP physicalDevice = device->physicalDevice;
    // DriverInstance::SP instance = physicalDevice->getInstanceHnalde;
      
    VmaAllocatorCreateInfo allocatorCreateInfo = {};
    allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_2;
    allocatorCreateInfo.physicalDevice   = physicalDevice->handle;
    allocatorCreateInfo.device           = device->handle;
    allocatorCreateInfo.instance         = device->getInstanceHandle();
    allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;
    allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vmaCreateAllocator(&allocatorCreateInfo, &handle);
    assert(handle);
    std::cout << "#vkn: allocator created" << std::endl;
  }
    
  void Allocator::destroy()
  {
    assert(handle);
    vmaDestroyAllocator(handle);
  }
    
  VkDeviceMemory
  LogicalDevice::allocateMemory(const VkMemoryRequirements &memReqs,
                                      VkMemoryPropertyFlags memoryPropertyFlags)
  {
    // VkMemoryPropertyFlags memoryPropertyFlags
    //   = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    
    VkMemoryAllocateInfo memAllocInfo{};
    memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAllocInfo.allocationSize = memReqs.size;
    // Find a memory type index that firts the properties of the image
    memAllocInfo.memoryTypeIndex
      = physicalDevice->getMemoryType(memReqs.memoryTypeBits,
                                      memoryPropertyFlags);

    VkDeviceMemory memory = 0;
    VK_CHECK(vkAllocateMemory(this->handle,
                              &memAllocInfo,
                              nullptr,
                              &memory),
             "could not allocate device memory");
    return memory;
  }

  void LogicalDevice::freeMemory(VkDeviceMemory memory)
  {
    if (!memory) return;
    vkFreeMemory(this->handle,memory,nullptr);
  }
  
  
  LogicalDevice::LogicalDevice(const PhysicalDevice::SP &physicalDevice)
    : physicalDevice(physicalDevice)
  {
    // std::vector<CustomQueueDescription> queue_descriptions;
    // queue_descriptions.insert(queue_descriptions.end(), info.queue_descriptions.begin(), info.queue_descriptions.end());
      
    // if (queue_descriptions.empty()) {
    //   for (uint32_t i = 0; i < physical_device.queue_families.size(); i++) {
    //     queue_descriptions.emplace_back(i, std::vector<float>{ 1.0f });
    //   }
    // }
      
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    // for (auto& desc : queue_descriptions) {
    VkDeviceQueueCreateInfo queue_create_info = {};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = physicalDevice->graphicsQueueFamilyIndex;
    queue_create_info.queueCount = 1;//static_cast<std::uint32_t>(desc.priorities.size());
    static const float priority = 1.f;
    queue_create_info.pQueuePriorities = &priority;//desc.priorities.data();
      
    queueCreateInfos.push_back(queue_create_info);
      
    // }

    bool needSER = false;

    std::vector<const char *> enabledDeviceExtensions = {
      VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME,
      VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
      VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
      VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
      VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
      VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
      VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
      VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME,
      VK_KHR_SPIRV_1_4_EXTENSION_NAME,
      VK_KHR_VULKAN_MEMORY_MODEL_EXTENSION_NAME,
      VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,
      VK_KHR_RAY_QUERY_EXTENSION_NAME,
    };

    if (needSER)
      enabledDeviceExtensions.push_back
        (VK_NV_RAY_TRACING_INVOCATION_REORDER_EXTENSION_NAME);

    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};
    accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;

    VkPhysicalDeviceRayTracingInvocationReorderFeaturesNV invocationReorderFeatures{};
    invocationReorderFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_INVOCATION_REORDER_FEATURES_NV;
    invocationReorderFeatures.rayTracingInvocationReorder = 1;//requestedFeatures.invocationReordering;
    invocationReorderFeatures.pNext = nullptr;
    if (needSER)
      accelerationStructureFeatures.pNext = &invocationReorderFeatures;
      

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtPipelineFeatures{};
    rtPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
    rtPipelineFeatures.pNext = &accelerationStructureFeatures;

    VkPhysicalDeviceRayQueryFeaturesKHR rtQueryFeatures{};
    rtQueryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
    rtQueryFeatures.pNext = &rtPipelineFeatures;

    VkPhysicalDeviceVulkan12Features deviceVulkan12Features;
    deviceVulkan12Features = {};
    deviceVulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    deviceVulkan12Features.pNext = &rtQueryFeatures;

    VkPhysicalDeviceVulkan11Features deviceVulkan11Features;
    deviceVulkan11Features = {};
    deviceVulkan11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    deviceVulkan11Features.pNext = &deviceVulkan12Features;    

    VkPhysicalDeviceFeatures2 deviceFeatures2;
    deviceFeatures2 = {};
    deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures2.pNext = &deviceVulkan11Features;


    // VkPhysicalDeviceFeatures deviceFeatures;
    // deviceFeatures = {};
    // deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES;
    // deviceFeatures.pNext = &deviceVulkan11Features2;

    // vkGetPhysicalDeviceFeatures(physicalDevice->handle, &deviceFeatures);
    vkGetPhysicalDeviceFeatures2(physicalDevice->handle, &deviceFeatures2);
      
    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = queueCreateInfos.size();
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.enabledExtensionCount = (uint32_t) enabledDeviceExtensions.size();
    createInfo.ppEnabledExtensionNames = enabledDeviceExtensions.data();
    
    createInfo.pEnabledFeatures = 0;
    createInfo.pNext = &deviceFeatures2;
      
        
    VK_CALL(CreateDevice(physicalDevice->handle, &createInfo, nullptr,
                         &this->handle),
            "could not create logical device from physical device");
    std::cout << "#vkn: logical device created" << std::endl;


    vkGetRayTracingShaderGroupHandles
      = (PFN_vkGetRayTracingShaderGroupHandlesKHR)
      vkGetDeviceProcAddr(handle, "vkGetRayTracingShaderGroupHandlesKHR");
    assert(vkGetRayTracingShaderGroupHandles);
      
    vkCreateRayTracingPipelines
      = (PFN_vkCreateRayTracingPipelinesKHR) 
      vkGetDeviceProcAddr(handle, "vkCreateRayTracingPipelinesKHR");
    assert(vkCreateRayTracingPipelines);

    vkCreateComputePipelines
      = (PFN_vkCreateComputePipelines) 
      vkGetDeviceProcAddr(handle, "vkCreateComputePipelines");
    assert(vkCreateComputePipelines);

    vkCmdTraceRays
      = (PFN_vkCmdTraceRaysKHR) 
      vkGetDeviceProcAddr(handle, "vkCmdTraceRaysKHR");
    assert(vkCmdTraceRays);

    vkGetBufferDeviceAddress
      = (PFN_vkGetBufferDeviceAddress) 
      vkGetDeviceProcAddr(handle, "vkGetBufferDeviceAddressKHR");
    assert(vkGetBufferDeviceAddress);

    vkGetAccelerationStructureBuildSizes
      = (PFN_vkGetAccelerationStructureBuildSizesKHR)
      vkGetDeviceProcAddr(handle, "vkGetAccelerationStructureBuildSizesKHR");
    assert(vkGetAccelerationStructureBuildSizes);
    
    vkCreateAccelerationStructure
      = (PFN_vkCreateAccelerationStructureKHR)
      vkGetDeviceProcAddr(handle, "vkCreateAccelerationStructureKHR");
    assert(vkCreateAccelerationStructure);
    
    vkDestroyAccelerationStructure
      = (PFN_vkDestroyAccelerationStructureKHR)
      vkGetDeviceProcAddr(handle, "vkDestroyAccelerationStructureKHR");
    assert(vkDestroyAccelerationStructure);

    vkGetAccelerationStructureDeviceAddress
      = (PFN_vkGetAccelerationStructureDeviceAddressKHR)
      vkGetDeviceProcAddr(handle, "vkGetAccelerationStructureDeviceAddressKHR");
    assert(vkGetAccelerationStructureDeviceAddress);
    
    vkCmdBuildAccelerationStructures
      = (PFN_vkCmdBuildAccelerationStructuresKHR)
      vkGetDeviceProcAddr(handle, "vkCmdBuildAccelerationStructuresKHR");
    assert(vkCmdBuildAccelerationStructures);
#if 0
    xxx
      = (PFN_xxxKHR)
      vkGetDeviceProcAddr(handle, "xxxKHR");
    assert(xxx);
#endif

      
    vkGetDeviceQueue(handle, physicalDevice->graphicsQueueFamilyIndex, 0,
                     &queue);
      
      
    allocator.create(this);
    std::cout << "#vkn: allocator created" << std::endl;


    createCommandPool();
    createCommandBuffer();
  }

  void LogicalDevice::track(Context *newContextOnThisDevice)
  {
    contextsOnThisDevice.insert(newContextOnThisDevice);
  }
  
  void LogicalDevice::forget(Context *contextLeavingThisDevice)
  {
    // auto it = std::find_if(contextsOnThisDevice.begin(),
    //                        contextsOnThisDevice.end(),
    //                        [&](std::weak_ptr<Context> ctx)
    //                        {
    //                          auto _ctx = ctx.lock();
    //                          return _ctx && _ctx.get() == contextLeavingThisDevice;
    //                        });
    // contextsOnThisDevice.erase(it);
    contextsOnThisDevice.erase(contextLeavingThisDevice);
  }

  void LogicalDevice::createCommandPool()
  {
    assert(commandPool == nullptr && "command pool already created!?");
      
    VkCommandPoolCreateInfo cmdPoolInfo = {};
    cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolInfo.queueFamilyIndex = physicalDevice->graphicsQueueFamilyIndex;
    cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CALL(CreateCommandPool(handle, &cmdPoolInfo, nullptr, &commandPool),
            "could not create default command pool on device");
    assert(commandPool);
      
    std::cout << "#vkn: command pool created" << std::endl;
  }

  void LogicalDevice::createCommandBuffer()
  {
    assert(commandBuffer == nullptr &&
           "command buffer already created!?");
    assert(commandPool != nullptr &&
           "must create command pool before createing comamnd buffer");
      
    VkCommandBufferAllocateInfo cmdBufAllocateInfo{};
    cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufAllocateInfo.commandBufferCount = 1;
      
    cmdBufAllocateInfo.commandPool = commandPool;
    VK_CALL(AllocateCommandBuffers(handle,
                                   &cmdBufAllocateInfo,
                                   &commandBuffer),
            "could not allocate default command buffer");
    std::cout << "#vkn: default command buffer created" << std::endl;

  }
    
  LogicalDevice::~LogicalDevice()
  {
    allocator.destroy();
    
    vkDestroyCommandPool(handle, commandPool, nullptr);

    vkDeviceWaitIdle(handle);
    vkDestroyDevice(handle, nullptr);
  }

  void LogicalDevice::terminate()
  {
    auto copyOfList = contextsOnThisDevice;
    for (auto ctx : copyOfList)
      ctx->terminate();
  }

} // ::vkn

