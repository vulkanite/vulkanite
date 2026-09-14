// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "vulkanite/LogicalDevice.h"
#include "vulkanite/Object.h"

namespace vkn {
    
  struct DeviceBuffer : public Object {
    typedef std::shared_ptr<DeviceBuffer> SP;
    DeviceBuffer(Context *context,
                 size_t sizeInBytes,
                 size_t alignment=0);
    ~DeviceBuffer() override;
    
    std::string toString() override { return "DeviceBuffer"; }
    
    static void copy(DeviceBuffer *dst, DeviceBuffer *src);
    static void copy(DeviceBuffer::SP dst, DeviceBuffer::SP src)
    { copy(dst.get(),src.get()); }

    VkDeviceAddress deviceAddress() const { return devPtr; }
    void upload(const void *data);
    
    void *map();
    void unmap();
    
    /*! free whatever device data we are currently holding. will not
        destroy the buffer class itself, but free all device data */
    void free();
    
    size_t alignment = 16;
      
    LogicalDevice::SP const device;
    size_t            const sizeInBytes;
    VkBuffer          handle = 0;
    VmaAllocation     allocation;
    VkDeviceAddress   devPtr = 0;
    void             *mapped = nullptr;
  };

  inline VkDeviceAddress alignAddress(VkDeviceAddress addr,
                                      size_t align)
  {
    size_t asInt = (size_t)addr;
    return (VkDeviceAddress)(align * (asInt+align-1)/align);
  }
    
} // ::vkn

