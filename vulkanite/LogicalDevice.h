// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "vulkanite/PhysicalDevice.h"
#define VMA_VULKAN_VERSION 1002000   // Vulkan 1.2
#include "vma/vk_mem_alloc.h"

namespace vkn {
    
  struct LogicalDevice;
  struct Context;
  
  struct Allocator {
    void create(LogicalDevice *device);
    void destroy();
      
    VmaAllocator             handle = 0;
  };
    
  struct LogicalDevice : public std::enable_shared_from_this<LogicalDevice> {
    typedef std::shared_ptr<LogicalDevice> SP;
      
    LogicalDevice(const PhysicalDevice::SP &physicalDevice);
    virtual ~LogicalDevice();

    void terminate();

    VkInstance getInstanceHandle() const { return physicalDevice->getInstanceHandle(); }
    VkDevice   getHandle() const { return handle; }
    
    VkDeviceMemory allocateMemory(const VkMemoryRequirements &memReqs,
                                  VkMemoryPropertyFlags properties);
    void freeMemory(VkDeviceMemory memory);

    PhysicalDevice::SP const physicalDevice;
    Allocator                allocator;
    VkDevice                 handle = 0;
    VkCommandPool   commandPool = 0;
    VkCommandBuffer commandBuffer = 0;
    VkQueue         queue = 0;

    // ------------------------------------------------------------------
    // infrastructure to track which contexts are alive on this device
    // ------------------------------------------------------------------
    void track(Context *newContextOnThisDevice);
    void forget(Context *contextLeavingThisDevice);
  private:
    /*! list of contexts that were created on this device, and haven't
        been released yet, so we can later on kill them as required */
    std::set<Context *> contextsOnThisDevice;
    // ------------------------------------------------------------------

  public:
    PFN_vkCreateComputePipelines vkCreateComputePipelines = 0;
    PFN_vkGetBufferDeviceAddress vkGetBufferDeviceAddress = 0;
    PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandles = 0;
    PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelines = 0;
    PFN_vkCmdTraceRaysKHR vkCmdTraceRays = 0;
    PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizes = 0;
    PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructures = 0;
    PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructure = 0;
    PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructure = 0;
    PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddress = 0;
  private:
    void createCommandPool();
    void createCommandBuffer();
  };

} // ::vkn

