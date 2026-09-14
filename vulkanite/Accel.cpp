// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#include "vulkanite/Accel.h"
#include "vulkanite/Context.h"

namespace vkn {

  Accel::Accel(Context *context,
               size_t sbtEntryCount)
    : Object(context),
      sbtEntryBegin(sbtEntryCount
                    ? context->sbtRangeAllocator.alloc(sbtEntryCount)
                    : -1),
      sbtEntryCount(sbtEntryCount)
  {
    LOG_CONSTRUCTION(Accel,this);
  }
  
  Accel::~Accel()
  {
    context->sbtRangeAllocator.release(sbtEntryBegin,sbtEntryCount);
    destroyAccel();

    LOG_DESTRUCTION(Accel,this);
  }
 

  uint64_t Accel::deviceAddress() const
  {
    if (handle == VK_NULL_HANDLE)
      return 0;

    VkAccelerationStructureDeviceAddressInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    info.accelerationStructure = handle;
    return device->vkGetAccelerationStructureDeviceAddress(device->getHandle(), &info);
  }


  void Accel::destroyAccel()
  {
    if (handle == VK_NULL_HANDLE)
      return;

    asBuffer = {};
    device->vkDestroyAccelerationStructure(device->getHandle(),
                                           handle, nullptr);
    handle = VK_NULL_HANDLE;
  }
  
}
