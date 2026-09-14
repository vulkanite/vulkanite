// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "vulkanite/Object.h"
#include "vulkanite/Buffer.h"
#include "vulkanite/GeomType.h"

namespace vkn {
  struct Context;

  struct Accel : Object {
    typedef std::shared_ptr<Accel> SP;

    Accel(Context *context,
          size_t sbtEntryCount);
    virtual ~Accel();

    virtual void build() = 0;

    void destroyAccel();
    uint64_t deviceAddress() const;
    
    DeviceBuffer::SP asBuffer;
    VkAccelerationStructureKHR handle = VK_NULL_HANDLE;
    
    const int    sbtEntryBegin;
    const size_t sbtEntryCount;
  };

  struct GeomAccel : public Accel {
    typedef std::shared_ptr<GeomAccel> SP;
    
    GeomAccel(Context *context,
              size_t sbtEntryCount)
      : Accel(context,sbtEntryCount) 
    {}
    virtual const std::vector<Geom::SP> &getGeoms() const = 0;
  };
  
} // ::vkn
