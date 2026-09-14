// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#include "vulkanite/Triangles.h"
#include "vulkanite/Context.h"

namespace vkn {

  TrianglesGeom::TrianglesGeom(GeomType *gt)
    : Geom(gt)
  {}
  
  TrianglesGeom::~TrianglesGeom()
  {}

  TrianglesGeomAccel::TrianglesGeomAccel(Context *context,
                       const std::vector<TrianglesGeom::SP> &geoms)
    : GeomAccel(context,geoms.size()),
      geoms(geoms)
  {
    LOG_CONSTRUCTION(TrianglesAccel,this);

    context->rtPipeline.track(this);
  }
  
  TrianglesGeomAccel::~TrianglesGeomAccel()
  {
    context->rtPipeline.forget(this);
    
    destroyAccel();

    LOG_DESTRUCTION(TrianglesAccel,this);
  }

  std::string TrianglesGeomAccel::toString()
  { return "TrianglesGeomAccel"; }

  
  void TrianglesGeomAccel::build()
  {
    destroyAccel();
    
    std::vector<VkAccelerationStructureGeometryKHR> vkGeoms;

    int numGeoms = geoms.size();
    std::vector<uint32_t> primCounts(numGeoms);
    std::vector<VkAccelerationStructureBuildRangeInfoKHR*>
      buildRangeInfoPointers(numGeoms);
    std::vector<VkAccelerationStructureBuildRangeInfoKHR>
      buildRangeInfos(numGeoms);
    for (int geomID=0;geomID<numGeoms;geomID++) {
      auto &geom = geoms[geomID];
      
      VkAccelerationStructureGeometryKHR vg = {};
      vg.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
      vg.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
      auto &vg_tris = vg.geometry.triangles;
      vg_tris.sType
        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
      vg_tris.vertexFormat             = VK_FORMAT_R32G32B32_SFLOAT;
      vg_tris.vertexData.deviceAddress = geom->vertices.d_address;
      vg_tris.vertexStride             = geom->vertices.stride;

      // "maxVertex is the number of vertices in vertexData minus one."
      // https://docs.vulkan.org/refpages/latest/refpages/source/VkAccelerationStructureGeometryTrianglesDataKHR.html
      vg_tris.maxVertex                = geom->vertices.count - 1;
      
      vg_tris.indexType               = VK_INDEX_TYPE_UINT32;
      // note, offset accounted for in range
      vg_tris.indexData.deviceAddress = geom->indices.d_address;
      vg_tris.transformData.deviceAddress = 0;

      vkGeoms.push_back(vg);
      
      auto &primCount = primCounts[geomID];
      primCount = geom->indices.count;
      
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
    // PING;
    // PRINT(asBuildSizeInfo.buildScratchSize);

    // ==================================================================
    // allocate device mem for scratch and accel
    // ==================================================================
    size_t scratchAlignment = device->physicalDevice->accelAlignment;
    // PING; PRINT(scratchAlignment);
    // size_t scratchAlignment = 128;
    DeviceBuffer scratchBuffer(context,
                               asBuildSizeInfo.buildScratchSize,
                               scratchAlignment);
    asBuffer = std::make_shared<DeviceBuffer>
      (context,asBuildSizeInfo.accelerationStructureSize,scratchAlignment);
      // (context,asBuildSizeInfo.accelerationStructureSize+accelAlignment);

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
  
} // ::vkn


