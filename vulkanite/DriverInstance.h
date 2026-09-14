// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "vulkanite/LogicalDevice.h"

namespace vkn {
  namespace driver {
    struct Instance {
      Instance();
      virtual ~Instance();
      
      /*! the actual vulkan driver instance handle */
      VkInstance handle = 0;
    };
  } // ::vkn::driver
  
  struct DriverInstance : public std::enable_shared_from_this<DriverInstance> {
    typedef std::shared_ptr<DriverInstance> SP;
    
    DriverInstance(VKNDeviceType deviceType);
    ~DriverInstance();

    VkInstance getHandle() const { return driverHandle->handle; };
    
    /*! called when APP calls vknTerminate() */
    void terminate();


    // ====================================================================
    // self-referencing, to allow api to hold a reference
    // ====================================================================
  public:
    void createSelfReference();
  private:
    DriverInstance::SP selfReference;

  private:
    // ====================================================================
    // low level helper functions
    // ====================================================================
    void createDriverHandle();

    
    /*! create a list of matching physical devices matchign the
        specified type(s) that can do ray tracing */
    std::vector<PhysicalDevice::SP> findDevices(VKNDeviceType deviceType);
    std::shared_ptr<driver::Instance> driverHandle;
  public:
    std::vector<LogicalDevice::SP>  devices;
  };

} // ::vkn
    

