// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#include "vulkanite/PhysicalDevice.h"
#include "vulkanite/DriverInstance.h"

namespace vkn {
    
  PhysicalDevice::PhysicalDevice(const std::shared_ptr<driver::Instance> &driverInstance,
                                 uint32_t vulkanGpuID,
                                 VkPhysicalDevice handle)
    : driverInstance(driverInstance),
      vulkanGpuID(vulkanGpuID),
      handle(handle)
  {
    // std::cout << "########### physical " << vulkanGpuID << " CREATED" << std::endl;
    VkPhysicalDeviceAccelerationStructurePropertiesKHR
      accelProps = {};
    accelProps.sType
      = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
    
    VkPhysicalDeviceProperties2 props {};
    props.sType
      = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    props.pNext = &accelProps;

    // VkPhysicalDeviceProperties properties;
    // vkGetPhysicalDeviceProperties(handle, &properties);
    vkGetPhysicalDeviceProperties2(handle, &props);
    VkPhysicalDeviceProperties &properties = props.properties;
    //    this->accelAlignment = accelProps.
    name = std::string(properties.deviceName);

    this->accelAlignment
      = accelProps.minAccelerationStructureScratchOffsetAlignment;
  
    switch(properties.deviceType) {
    case VK_PHYSICAL_DEVICE_TYPE_OTHER:
      this->type = PhysicalDevice::Other;
      break;
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
      this->type = PhysicalDevice::IntegratedGPU;
      break;
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
      this->type = PhysicalDevice::DiscreteGPU;
      break;
    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
      this->type = PhysicalDevice::VirtualGPU;
      break;
    case VK_PHYSICAL_DEVICE_TYPE_CPU:
      this->type = PhysicalDevice::CPU;
      break;
    default:
      assert(0 && "unhandled physical device type");
    };

    // ==================================================================
    // PRINT((int*)properties.apiVersion);
    // ==================================================================

    vkGetPhysicalDeviceMemoryProperties(this->handle,
                                        &memoryProperties);
    // vkGetPhysicalDeviceAccelerationStructureProperties(this->handle,
    //                                     &accelProperties);
      
    uint32_t numExtensions;
    VK_CALL(EnumerateDeviceExtensionProperties(this->handle, nullptr,
                                               &numExtensions, nullptr),
            "could not query number of physical device extensions");
      
    std::vector<VkExtensionProperties> extensions(numExtensions);
    VK_CALL(EnumerateDeviceExtensionProperties(this->handle, nullptr, &numExtensions,
                                               extensions.data()),
            "could not query actual list of phyiscal device extensions");
    for (auto ext : extensions)
      this->extensions.insert(std::string(ext.extensionName));
      

    // ==================================================================
    // device queue families
    // ==================================================================
    uint32_t numQueueFamilieProperties;
    vkGetPhysicalDeviceQueueFamilyProperties
      (handle,&numQueueFamilieProperties,nullptr);
    std::vector<VkQueueFamilyProperties>
      queueFamilyProperties(numQueueFamilieProperties);
    vkGetPhysicalDeviceQueueFamilyProperties
      (handle,&numQueueFamilieProperties,queueFamilyProperties.data());
    
    for (int idx=0;idx<queueFamilyProperties.size();idx++) {
      auto qfp = queueFamilyProperties[idx];
      // PRINT((int*)qfp.queueFlags);
      // PRINT(qfp.queueCount);
      if (qfp.queueFlags & (VK_QUEUE_GRAPHICS_BIT |
                            VK_QUEUE_COMPUTE_BIT |
                            VK_QUEUE_TRANSFER_BIT)) {
        std::cout << "#vkn: found " << toString() << ": gfx|cmpt queue count " << qfp.queueCount << std::endl;
        // std::cout << toString() << ": FOUND graphics|compute queue, idx = " << idx << ", count = " << qfp.queueCount << std::endl;
        graphicsQueueFamilyIndex = idx;
        break;
      }
    }
  }

  PhysicalDevice::~PhysicalDevice()
  {
    // std::cout << "########### physical " << vulkanGpuID << " DYING" << std::endl;
    driverInstance.reset();
  }
  
  VkInstance PhysicalDevice::getInstanceHandle() const
  { return driverInstance->handle; }
      

  // memory type bits is a bitmask and contains one bit set for every
  // supported memory type. Bit i is set if and only if the memory type i in
  // the memory properties is supported.
  uint32_t PhysicalDevice::getMemoryType(uint32_t typeBits,
                                         VkMemoryPropertyFlags properties)
  {
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
      if ((typeBits & 1) == 1) {
        if ((memoryProperties.memoryTypes[i].propertyFlags & properties)
            == properties) {
          // if (memTypeFound) {
          //   *memTypeFound = true;
          // }
          return i;
        }
      }
      typeBits >>= 1;
    }
    
    // if (memTypeFound) {
    //   *memTypeFound = false;
    return 0;
    // } else {
    //   LOG_ERROR("Could not find a matching memory type");
    // }
    // return -1;
  }
  

  bool PhysicalDevice::hasExtensions
  (const std::vector<const char *> &listOfExtensions) const
  {
    for (auto ext : listOfExtensions)
      if (this->extensions.find(std::string(ext)) == this->extensions.end()) {
        return false;
      }
    return true;
  }

  bool PhysicalDevice::canDoInlineRayQueries() const
  {
    return canDoRayTracing() &&
      hasExtensions({VK_KHR_RAY_QUERY_EXTENSION_NAME});
  }
  
  bool PhysicalDevice::canDoSER() const
  {
    return canDoRayTracing() &&
      hasExtensions({VK_NV_RAY_TRACING_INVOCATION_REORDER_EXTENSION_NAME});
  }
  
  bool PhysicalDevice::canDoRayTracing() const
  {
    // std::cout << "PHYSICAL " << name << " checking on extensions: " << std::endl;
    std::vector<const char *> rtExtensions = {
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
    };
    return hasExtensions(rtExtensions);
  }
  
  
  std::string PhysicalDevice::toString() const
  {
    std::stringstream ss;
    ss << "PhysicalDevice{name="<<name
       <<"\n\ttype="<<to_string(type)
       << ",rt="<<(canDoRayTracing()?"yes":"no")
       << ",inline="<<(canDoInlineRayQueries()?"yes":"no")
       << ",SER="<<(canDoSER()?"yes":"no")
       <<"}";
    return ss.str();
  }
  
  std::string to_string(PhysicalDevice::Type type)
  {
    switch(type) {
    case PhysicalDevice::IntegratedGPU: return "IntegratedGPU";
    case PhysicalDevice::DiscreteGPU:   return "DiscreteGPU";
    case PhysicalDevice::VirtualGPU:    return "VirtualGPU";
    case PhysicalDevice::CPU:           return "CPU";
    case PhysicalDevice::Other:         return "Other";
    default:
      assert(0 && "un-handled device type");
      return "un-handled type #"+std::to_string((int)type);
    };
  }

  
} // ::vkn
