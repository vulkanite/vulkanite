// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "vulkanite/vkn-common.h"

namespace vkn {
  namespace driver {
    struct Instance;
  };
  
  struct PhysicalDevice {
    typedef std::shared_ptr<PhysicalDevice> SP;

    typedef enum { IntegratedGPU, DiscreteGPU, VirtualGPU, CPU, Other } Type;
    
    PhysicalDevice(const std::shared_ptr<driver::Instance> &driverInstance,
                   uint32_t vulkanGpuID,
                   VkPhysicalDevice handle);
    virtual ~PhysicalDevice();
    std::string toString() const;

    /*! checks if the device's extension list contains all of the
      VK_... extensions listed in the given vector */
    bool hasExtensions(const std::vector<const char *> &listOfExtensions) const;
    
    /*! returns true if the device has all the extensions to build a
      'classical' ray tracing pipeline (with acceleration
      structures, device programs, etc, but not necessarily inline
      ray queries or SER) */
    bool canDoRayTracing() const;

    /*! returns if this device can do ray tracing AND inline ray queries */
    bool canDoInlineRayQueries() const;
    
    /*! returns if this device can do ray tracing AND shader execution
      reordering (SER) */
    bool canDoSER() const;
    
    // memory type bits is a bitmask and contains one bit set for every
    // supported memory type. Bit i is set if and only if the memory type i in
    // the memory properties is supported.
    uint32_t getMemoryType(uint32_t typeBits,
                           VkMemoryPropertyFlags properties);
    
    uint32_t         vulkanGpuID;
    std::string      name;
    VkPhysicalDevice handle;
    Type             type;
    int              graphicsQueueFamilyIndex = -1;
    /*! list of device extensions, in plain std::string's */
    std::set<std::string> extensions;
    VkPhysicalDeviceMemoryProperties memoryProperties;
    size_t accelAlignment;
    VkInstance getInstanceHandle() const;
    
    /*! the instance we were created under */
    std::shared_ptr<driver::Instance> driverInstance;
  };

  std::string to_string(PhysicalDevice::Type type);
    
} // ::vkn
