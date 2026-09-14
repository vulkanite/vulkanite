// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#include "vulkanite/InstanceAccel.h"
#include "vulkanite/Context.h"

namespace vkn {

  VKNFloat4x3 unitTransform()
  {
    VKNFloat4x3 v;
    v.v[0] = { 1.f, 0.f, 0.f, 0.f };
    v.v[1] = { 0.f, 1.f, 0.f, 0.f };
    v.v[2] = { 0.f, 0.f, 1.f, 0.f };
    return v;
  }

  InstanceAccel::InstanceAccel(Context *context,
                               const std::vector<GeomAccel::SP> &accels)
    : Accel(context,accels.size()),
      accels(accels)
  {
    LOG_CONSTRUCTION(InstanceAccel,this);
  }
  
  InstanceAccel::~InstanceAccel()
  {
    LOG_DESTRUCTION(InstanceAccel,this);
    instanceBuffer = {};
    asHandlesBuffer = {};
  }

  std::string InstanceAccel::toString()
  { return "InstanceAccel"; }

  void InstanceAccel::setTransformsHost(VKNFloat4x3 *transforms)
  {
    hostTransforms.resize(accels.size());
    if (transforms) {
      memcpy(hostTransforms.data(),transforms,
             accels.size()*sizeof(*transforms));
    } else {
      for (auto &xf : hostTransforms) {
        xf = unitTransform();
      }
    }
  }

  void InstanceAccel::build()
  {
    destroyAccel();
    if (accels.empty())
      return;
    
    std::vector<VkAccelerationStructureKHR> asHandles;
    for (auto as : accels)
      asHandles.push_back(as->handle);
    asHandlesBuffer = std::make_shared<DeviceBuffer>
      (context,asHandles.size()*sizeof(asHandles[0]));
    asHandlesBuffer->upload(asHandles.data());
                            
    std::vector<VkAccelerationStructureGeometryKHR> vkGeoms(1);

    int instanceOffset = sbtEntryBegin;
    uint64_t visibilityMasksAddress = -1;
    uint64_t transformsAddress = -1;
    
    int numInstances = asHandles.size();
    instanceBuffer = std::make_shared<DeviceBuffer>
      (context,numInstances*sizeof(VkAccelerationStructureInstanceKHR));
    
    std::vector<uint32_t> primCounts(1);
    primCounts[0] = numInstances;
    
    std::vector<VkAccelerationStructureBuildRangeInfoKHR>
      buildRangeInfos(1);
    auto &buildRangeInfo = buildRangeInfos[0];
    buildRangeInfo.primitiveCount = numInstances;
    buildRangeInfo.primitiveOffset = 0;
    buildRangeInfo.firstVertex = 0;
    buildRangeInfo.transformOffset = 0;

    std::vector<VkAccelerationStructureBuildRangeInfoKHR*>
      buildRangeInfoPointers(1);
    buildRangeInfoPointers[0] = &buildRangeInfos[0];

    VkAccelerationStructureInstanceKHR *instances
      = (VkAccelerationStructureInstanceKHR *)instanceBuffer->map();
    for (int instID=0;instID<numInstances;instID++) {
      auto instance = instances+instID;
      instance->mask = 0xFF;
      instance->instanceCustomIndex = instID;
      instance->instanceShaderBindingTableRecordOffset
        = ///*todo*/0;//instanceOffset + blasOffsets[i];
        accels[instID]->sbtEntryBegin;
      
      instance->flags = 0;
      instance->accelerationStructureReference
        = accels[instID]->deviceAddress();
      if (hostTransforms.empty())
        instance->transform = {
          {
            {1.0f, 0.0f, 0.0f, 0.0f}, // First row
            {0.0f, 1.0f, 0.0f, 0.0f}, // Second row
            {0.0f, 0.0f, 1.0f, 0.0f}  // Third row
          }            
        };
      else
        (VKNFloat4x3&)instance->transform
          = hostTransforms[instID];

    }
    instanceBuffer->unmap();

    VkAccelerationStructureGeometryKHR &asGeom = vkGeoms[0];// = {};
    asGeom.sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    asGeom.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    asGeom.flags        = VK_GEOMETRY_OPAQUE_BIT_KHR;

    auto &asInsts              = asGeom.geometry.instances;
    asInsts.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    asInsts.arrayOfPointers    = VK_FALSE;
    asInsts.data.deviceAddress = instanceBuffer->deviceAddress();//asInstances->deviceAddress();
    
    VkAccelerationStructureBuildGeometryInfoKHR asBuildInfo = {};
    asBuildInfo.sType
      = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    asBuildInfo.type
      = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    asBuildInfo.flags
      = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR
      // | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR
      // | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR
      ;
    asBuildInfo.geometryCount = (uint32_t)vkGeoms.size();
    asBuildInfo.pGeometries   = vkGeoms.data();

    VkAccelerationStructureBuildSizesInfoKHR asBuildSizeInfo = {};
    asBuildSizeInfo.sType
      = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    device->vkGetAccelerationStructureBuildSizes
      (device->getHandle(),
       VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
       &asBuildInfo,
       primCounts.data(),
       &asBuildSizeInfo);

    // ==================================================================
    // allocate device mem for scratch and accel
    // ==================================================================
    size_t scratchAlignment = device->physicalDevice->accelAlignment;
    DeviceBuffer scratchBuffer(context,
                               asBuildSizeInfo.buildScratchSize,
                               scratchAlignment);
    asBuffer = std::make_shared<DeviceBuffer>
      (context,asBuildSizeInfo.accelerationStructureSize);

    // ==================================================================
    // CREATE the actual accel
    // ==================================================================
    VkAccelerationStructureCreateInfoKHR asCreateInfo{};
    asCreateInfo.sType
      = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    asCreateInfo.buffer
      = asBuffer->handle;
    asCreateInfo.size
      =  asBuildSizeInfo.accelerationStructureSize;
    asCreateInfo.type
      = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    VK_DEVICE_CALL(CreateAccelerationStructure(device->getHandle(),
                                               &asCreateInfo,
                                               nullptr,
                                               &handle),
                   "error in createaccel");


    // ==================================================================
    // launch build
    // ==================================================================
    asBuildInfo.mode
      = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    asBuildInfo.dstAccelerationStructure  = this->handle;
    asBuildInfo.scratchData.deviceAddress = scratchBuffer.deviceAddress();

    VkCommandBufferBeginInfo cmdBufBI{};
    cmdBufBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    VK_CALL(BeginCommandBuffer(device->commandBuffer, &cmdBufBI),
            "error beginning cmd buffer for accel build");
    // cannot error:
    device->vkCmdBuildAccelerationStructures
      (device->commandBuffer, 1, &asBuildInfo,
       buildRangeInfoPointers.data());

    VK_CALL(EndCommandBuffer(device->commandBuffer),
            "error ending cmd buffer for accel build");

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = NULL;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = nullptr;
    submitInfo.pWaitDstStageMask = nullptr;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &device->commandBuffer;
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.pSignalSemaphores = nullptr;

    VK_CALL(QueueSubmit(device->queue, 1, &submitInfo, VK_NULL_HANDLE),
            "error submitting raygen launch command(s) to queue");
    VK_CALL(QueueWaitIdle(device->queue),
            "error waiting for queue to finish raygen launch copy");
  }
  
}
