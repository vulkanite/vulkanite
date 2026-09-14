// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#include "vulkanite/UserGeom.h"
#include "vulkanite/Context.h"
#include "vulkanite/vulkanite.h"

namespace vkn {

  UserGeom::UserGeom(GeomType *gt)
    : Geom(gt)
  {}
  
  UserGeom::~UserGeom() {}



  UserGeomAccel::UserGeomAccel(Context *context,
                       const std::vector<UserGeom::SP> &geoms)
    : GeomAccel(context,geoms.size()),
      geoms(geoms)
  {
    LOG_CONSTRUCTION(UserAccel,this);

    context->rtPipeline.track(this);
    for (auto geom : geoms) PRINT(geom->primCount);
  }
  

  UserGeomAccel::~UserGeomAccel()
  {
    context->rtPipeline.forget(this);
    
    destroyAccel();

    LOG_DESTRUCTION(UserAccel,this);
  }

  std::string UserGeomAccel::toString()
  { return "UserGeomAccel"; }

  
  void UserGeomAccel::build()
  {
    destroyAccel();
    
    std::vector<VkAccelerationStructureGeometryKHR> vkGeoms;

    int numGeoms = geoms.size();
    std::vector<uint32_t> primCounts(numGeoms);
    std::vector<VkAccelerationStructureBuildRangeInfoKHR*>
      buildRangeInfoPointers(numGeoms);
    std::vector<VkAccelerationStructureBuildRangeInfoKHR>
      buildRangeInfos(numGeoms);
    size_t totalPrims = 0;
    size_t maxUserGeomSize = 0;
    for (auto geom : geoms) {
      if (geom) totalPrims += geom->primCount;
      maxUserGeomSize = std::max(maxUserGeomSize,geom->type->dataSize);
    }
    assert(totalPrims > 0);
    DeviceBuffer geomDataBuffer(context,
                                maxUserGeomSize,
                                16);
    DeviceBuffer boundsBuffer(context,
                              totalPrims*sizeof(VKNBox3),
                              8);
    VKNBox3 *d_bounds = (VKNBox3*)boundsBuffer.deviceAddress();
    
    d_bounds = (VKNBox3*)boundsBuffer.deviceAddress();
    for (int geomID=0;geomID<numGeoms;geomID++) {
      auto &geom = geoms[geomID];
      assert(geom);
      assert(geom->type);
      assert(geom->type->boundsKernel);
      geomDataBuffer.upload(geom->data.data());
      VKNBoundsKernelArgs kernelData;
      kernelData.primCount = geom->primCount;
      kernelData.primBounds = d_bounds;
      kernelData.geomData = (void*)geomDataBuffer.deviceAddress();
      
      glm::ivec3 dims;      
      dims.x = 1024;
      dims.y = divRoundUp(geom->primCount,dims.x);
      dims.z = 1;
      geom->type->boundsKernel->launch(dims,&kernelData);
      
      d_bounds += kernelData.primCount;
    }

    // ==================================================================
    // bounds are all known, built
    // ==================================================================
    d_bounds = (VKNBox3*)boundsBuffer.deviceAddress();
    for (int geomID=0;geomID<numGeoms;geomID++) {
      auto &geom = geoms[geomID];
      
      VkAccelerationStructureGeometryKHR vg = {};
      vg.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
      vg.geometryType = VK_GEOMETRY_TYPE_AABBS_KHR;
      auto &vg_geom = vg.geometry.aabbs;
      vg_geom.sType
        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
      vg_geom.pNext = 0;
      vg_geom.data.deviceAddress = (VkDeviceAddress)d_bounds;
      vg_geom.stride = 6*sizeof(float);
      d_bounds += geom->primCount;
      
      vkGeoms.push_back(vg);
      
      auto &primCount = primCounts[geomID];
      primCount = geom->primCount;
      
      auto &buildRangeInfo = buildRangeInfos[geomID];
      buildRangeInfo.primitiveCount = primCount;
      buildRangeInfo.primitiveOffset = 0;
      buildRangeInfo.firstVertex = 0;
      buildRangeInfo.transformOffset = 0;

      auto &buildRangeInfoPointer = buildRangeInfoPointers[geomID];
      buildRangeInfoPointer = &buildRangeInfo;
    }

    VkAccelerationStructureBuildGeometryInfoKHR asBuildInfo = {};
    asBuildInfo.sType
      = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    asBuildInfo.type
      = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
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
      (context,asBuildSizeInfo.accelerationStructureSize,scratchAlignment);

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
      = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    device->vkCreateAccelerationStructure
      (device->getHandle(), &asCreateInfo, nullptr, &handle);


    // ==================================================================
    // launch build
    // ==================================================================
    asBuildInfo.mode
      = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    asBuildInfo.dstAccelerationStructure  = this->handle;
    asBuildInfo.scratchData.deviceAddress
      = scratchBuffer.deviceAddress();
    // asBuildInfo.scratchData.deviceAddress
    //   = alignAddress(scratchBuffer.deviceAddress(),scratchAlignment);
    // PRINT((int*)asBuildInfo.scratchData.deviceAddress);

    VkCommandBufferBeginInfo cmdBufBI{};
    cmdBufBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    VK_CALL(BeginCommandBuffer(device->commandBuffer, &cmdBufBI),
            "error beginning cmd buffer for accel build");
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
