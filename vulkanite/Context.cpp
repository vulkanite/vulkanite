// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#include "vulkanite/PhysicalDevice.h"
#include "vulkanite/Buffer.h"
#include "vulkanite/RayGen.h"
#include "vulkanite/MissProg.h"
#include "vulkanite/GeomType.h"
#include "vulkanite/Compute.h"
#include "vulkanite/Context.h"
#include "vulkanite/Triangles.h"
#include "vulkanite/UserGeom.h"
#include "vulkanite/InstanceAccel.h"
#include "vulkanite/Image.h"
#include "vulkanite/Sampler.h"

namespace vkn {

  size_t roundUpToMultipleOf(size_t a, size_t b)
  { return a*divRoundUp(b,a); }

  size_t nextMultipleOf(size_t a, size_t b)
  { return a*divRoundUp(b,a); }
  
  // =============================================================================

  Context::Context(LogicalDevice::SP device,
                   size_t pushConstantsSize)
    : device(device),
      rtPipeline(this),
      pushConstantsSize(std::max(pushConstantsSize,(size_t)4)),
      uniformBuffersPool(device,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER/* _DYNAMIC*/),
      uniformBuffersLayout(device),
      uniformBuffersDS(device)
  {
    device->track(this);

    rayGenRecordsBuffer
      = std::make_shared<DeviceBuffer>(this,1024);
    hitGroupRecordsBuffer
      = std::make_shared<DeviceBuffer>(this,1024);

    uint32_t dummyTexels[4] = {0,0,0,0};
    defaultImage
      = std::make_shared<Image>(this,VKN_TEXEL_FORMAT_RGBA8,2,2,dummyTexels);
    
    defaultSampler =
      std::make_shared<Sampler>(this,defaultImage,VKN_TEXTURE_LINEAR,
                                VKN_SAMPLER_CLAMP,VKN_SAMPLER_CLAMP,
                                nullptr,false);
  }
  
  Context::~Context()
  {
    std::cout << "#vkn: ~Context winding down" << std::endl;
    rayGenRecordsBuffer = {};
    missProgRecordsBuffer = {};
    hitGroupRecordsBuffer = {};
    
    defaultSampler = {};
    defaultImage = {};
    
    // todo: kill pipeline ....(?)
    device->forget(this);
    device = {};
  }
  
  void Context::terminate()
  {
    std::cout << "#vkn: Context::terminate()" << std::endl;
    bool verboseDangle = false;
    
    if (!appUseCount.empty()) {
      if (verboseDangle)
        std::cout << "#vkn: Context asked to explicitly terminate, but "
                  << "some object still have dangling app references" << std::endl;
      for (auto obj : appUseCount) {
        if (verboseDangle)
          std::cout << " - " << obj.first->toString()
                    << " (dangling count " << obj.second.first << ")" << std::endl;
      }
      if (verboseDangle)
        std::cout << "#vkn: manually releasing dangling app refs for" << std::endl;
      for (auto &obj : appUseCount) {
        if (verboseDangle)
          std::cout << " - " << obj.first->toString() << std::endl;
        obj.second = {};
      }
      appUseCount.clear();
    }
    selfReference = {};
  }
  
  template<typename T>
  T *addAppRef(const std::shared_ptr<T> &object);

  

  void Context::rebuildDescriptors()
  {
    std::vector<VkDescriptorSetLayoutBinding> bindings = {
      {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER/*_DYNAMIC*/, 1,
       VK_SHADER_STAGE_RAYGEN_BIT_KHR
       |
       VK_SHADER_STAGE_COMPUTE_BIT
       , nullptr}
    };

    static bool created = false;
    if (!created) {
      uniformBuffersPool.create((int)bindings.size());
      uniformBuffersLayout.create(bindings);
      uniformBuffersDS.create(uniformBuffersPool,uniformBuffersLayout);
      created = true;
    }

  }
  
  void Context::buildPipeline()
  {
    rtPipeline.build();
  }

  template<typename T>
  T *Context::addAppRef(const std::shared_ptr<T> &object)
  {
    auto &pair = appUseCount[object.get()];
    assert(pair.first == 0);
    pair.first = 1;
    pair.second = object;
    return (T*)object.get();
  }

  void Context::buildSBT()
  {
    auto pipeline = &this->rtPipeline;

    const size_t sbtHeaderSize = pipeline->properties.shaderGroupHandleSize;
    const size_t sbtAlignment  = pipeline->properties.shaderGroupBaseAlignment;
      
    // ====================== compute max sizes ======================
    // ----------- raygen -----------
    size_t maxRayGenDataSize = 0;
    for (auto rg : pipeline->rayGens.elements)
      if (rg) maxRayGenDataSize = std::max(maxRayGenDataSize,rg->type->dataSize);
    assert(maxRayGenDataSize > 0);

    const size_t rayGenRecordSize
      = nextMultipleOf(sbtAlignment,sbtHeaderSize+maxRayGenDataSize);
    
    // ----------- missprog -----------
    size_t maxMissProgDataSize = 0;
    assert(!pipeline->missProgs.elements.empty());
    for (auto rg : pipeline->missProgs.elements)
      if (rg) maxMissProgDataSize = std::max(maxMissProgDataSize,rg->type->dataSize);

    const size_t missProgRecordSize
      = nextMultipleOf(sbtAlignment,sbtHeaderSize+maxMissProgDataSize);
    
    // ----------- hitgroup -----------
    size_t maxHitGroupDataSize = 0;
    for (auto gt : pipeline->geomTypes)
      if (gt) maxHitGroupDataSize = std::max(maxHitGroupDataSize,gt->dataSize);
    if (maxHitGroupDataSize == 0) {
      std::cout << "#vkn::buildSBT(): max hit group data size is 0!?" << std::endl;
      maxHitGroupDataSize = 1;
    }
    assert(maxHitGroupDataSize > 0);
    
    const size_t hitGroupRecordSize
      = nextMultipleOf(sbtAlignment,sbtHeaderSize+maxHitGroupDataSize);
    assert(hitGroupRecordSize > 0);
    
    // ====================== alloc buffers ======================
    int numRayGens = pipeline->rayGens.elements.size();
    rayGenRecordsBuffer
      = std::make_shared<DeviceBuffer>(this,numRayGens*rayGenRecordSize);
    
    int numMissProgs = pipeline->missProgs.elements.size();
    missProgRecordsBuffer
      = std::make_shared<DeviceBuffer>(this,numMissProgs*missProgRecordSize);
    
    int numHitGroups = sbtRangeAllocator.maxAllocedID;
    if (numHitGroups == 0) {
      std::cout << "#vkn: no hit groups set!?" << std::endl;
      numHitGroups = 1;
    }
    
    hitGroupRecordsBuffer
      = std::make_shared<DeviceBuffer>(this,numHitGroups*hitGroupRecordSize);

    // ====================== write buffers ======================
    { // ----------- raygen -----------
      uint8_t *mapped = (uint8_t*)rayGenRecordsBuffer->map();
      for (int rgID=0;rgID<numRayGens;rgID++) {
        RayGen *rg = pipeline->rayGens.elements[rgID];
        if (!rg) continue;
        uint8_t *record = mapped + rgID*rayGenRecordSize;
        pipeline->writeHeader(record,rg->type->shaderGroupIndex);
        rg->writeData(record+sbtHeaderSize);
      }
      rayGenRecordsBuffer->unmap();
    }

    { // ----------- missprog -----------
      uint8_t *mapped = (uint8_t*)missProgRecordsBuffer->map();
      for (int mpID=0;mpID<numMissProgs;mpID++) {
        MissProg *mp = pipeline->missProgs.elements[mpID];
        if (!mp) continue;
        uint8_t *record = mapped + mpID*missProgRecordSize;
        pipeline->writeHeader(record,mp->type->shaderGroupIndex);
        mp->writeData(record+sbtHeaderSize);
      }
      missProgRecordsBuffer->unmap();
    }

    // ----------- hitGroup(s) -----------
    {
      uint8_t *mapped = (uint8_t*)hitGroupRecordsBuffer->map();
      for (auto geomGroup : rtPipeline.geomAccels) {
      // for (auto geomGroup : rtPipeline.trianglesGeomAccels) {
        int hgID = geomGroup->sbtEntryBegin;
        for (auto geom : geomGroup->getGeoms()) {
          assert(geom);
          uint8_t *record = mapped + hgID*hitGroupRecordSize;
          pipeline->writeHeader(record,geom->type->shaderGroupIndex);
          geom->writeData(record+sbtHeaderSize);
          ++hgID;
        }
      }
      hitGroupRecordsBuffer->unmap();
    }
    
    // ====================== write buffers ======================
    { // ----------- raygen -----------
      sbt.rayGensTable.deviceAddress = rayGenRecordsBuffer->deviceAddress();
      sbt.rayGensTable.stride        = rayGenRecordSize;
      sbt.rayGensTable.size          = numRayGens*rayGenRecordSize;
    }
    { // ----------- missprog -----------
      sbt.missProgsTable.deviceAddress = missProgRecordsBuffer->deviceAddress();
      sbt.missProgsTable.stride        = missProgRecordSize;
      sbt.missProgsTable.size          = numMissProgs*missProgRecordSize;
    }
    { // ----------- hitgroup -----------
      sbt.hitGroupsTable.deviceAddress = hitGroupRecordsBuffer->deviceAddress();
      sbt.hitGroupsTable.stride        = hitGroupRecordSize;
      sbt.hitGroupsTable.size          = numHitGroups*hitGroupRecordSize;
    }
    
    sbt.callablesTable  = sbt.rayGensTable;
  }
 
  ShaderModule *Context::createShaderModule(const std::vector<uint8_t> &moduleData)
  {
    ShaderModule::SP shaderModule
      = std::make_shared<ShaderModule>(&rtPipeline,moduleData);
    return addAppRef(shaderModule);
  }

  VKNRayGen Context::createRayGen(ShaderModule *shaderModule,
                                  const char *entryPoint,
                                  size_t sbtSize)
  {
    RayGenType::SP rgt
      = std::make_shared<RayGenType>(&rtPipeline,shaderModule,entryPoint,sbtSize);
    RayGen::SP rg
      = std::make_shared<RayGen>(rgt.get());
    return (VKNRayGen)addAppRef(rg);
  }
  
  VKNCompute Context::createCompute(ShaderModule *shaderModule,
                                  const char *entryPoint,
                                  size_t sbtSize)
  {
    Compute::SP rg
      = std::make_shared<Compute>(shaderModule,entryPoint,sbtSize);
    return (VKNCompute)addAppRef(rg);
  }
  
  VKNMissProg Context::createMissProg(ShaderModule *shaderModule,
                                  const char *entryPoint,
                                  size_t sbtSize)
  {
    MissProgType::SP rgt
      = std::make_shared<MissProgType>(&rtPipeline,shaderModule,entryPoint,sbtSize);
    MissProg::SP rg
      = std::make_shared<MissProg>(rgt.get());
    return (VKNMissProg)addAppRef(rg);
  }
  
  VKNGeomType Context::createGeomType(VKNGeomKind kind,
                                      size_t sbtSize)
  {
    GeomType::SP gt
      = std::make_shared<GeomType>(&rtPipeline,kind,sbtSize);
    return (VKNGeomType)addAppRef(gt);
  }
  
  VKNGeom Context::createGeom(GeomType *gt)
  {
    return (VKNGeom)addAppRef(gt->createInstance());
  }
  
  VKNAccel Context::createTrianglesAccel(int numGeoms,
                                         TrianglesGeom **geoms)
  {
    std::vector<TrianglesGeom::SP> meshes;
    for (int i=0;i<numGeoms;i++)
      meshes.push_back(geoms[i]->asSP<TrianglesGeom>());
    TrianglesGeomAccel::SP accel
      = std::make_shared<TrianglesGeomAccel>(this,meshes);
    return (VKNAccel)addAppRef(accel);
  }
  
  VKNAccel Context::createUserGeomAccel(int numGeoms,
                                        UserGeom **geoms)
  {
    std::vector<UserGeom::SP> meshes;
    for (int i=0;i<numGeoms;i++)
      meshes.push_back(geoms[i]->asSP<UserGeom>());
    UserGeomAccel::SP accel
      = std::make_shared<UserGeomAccel>(this,meshes);
    return (VKNAccel)addAppRef(accel);
  }
  
  VKNAccel Context::createInstanceAccel(int numInstances,
                                        GeomAccel **_accels)
  {
    std::vector<GeomAccel::SP> accels;
    for (int i=0;i<numInstances;i++)
      accels.push_back(_accels[i]->asSP<GeomAccel>());
    InstanceAccel::SP tlas 
      = std::make_shared<InstanceAccel>(this,accels);
    return (VKNAccel)addAppRef(tlas);
  }
  

  VKNBuffer Context::createDeviceBuffer(size_t numBytes,
                                        const void *initData)
  {
    DeviceBuffer::SP buf
      = std::make_shared<DeviceBuffer>(this,numBytes);
    if (initData) 
      buf->upload(initData);

    return (VKNBuffer)addAppRef(buf);
  }


  VKNBuffer hack_syncBuffer = 0;
  
  
  void Context::launch(RayGen *rg,
                       MissProg *mp,
                       int dims_x, int dims_y, int dims_z,
                       const void *pushConstantData)
  {
    static int numLaunchesDone = 0;
    bool firstTime = (numLaunchesDone++ == 0);
    
    static int dummyValue = -1;
    if (pushConstantData == nullptr)
      pushConstantData = &dummyValue;

    if (firstTime)
      std::cout << "vkn::launch()" << std::endl;
    VkCommandBufferBeginInfo cmdBufBI{};
    cmdBufBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    VK_CALL(BeginCommandBuffer(device->commandBuffer, &cmdBufBI),
            "error beginning cmd buffer for raygen launch");

    rtPipeline.samplers.update();
    
    if (uniformBuffersDS.handle == 0) {
      // std::cout << "NULL uniform buffer desc ... rebuilding descriptors...." << std::endl;
      rebuildDescriptors();
    }
    std::vector<VkDescriptorSet> descriptorSets = {
      uniformBuffersDS.handle,
      rtPipeline.samplers.descriptorSet.handle,
      rtPipeline.image2Ds.descriptorSet.handle
    };
    vkCmdBindDescriptorSets(device->commandBuffer,
                            VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
                            rtPipeline.layout,
                            0,
                            (uint32_t)descriptorSets.size(), descriptorSets.data(),
                            0,nullptr);
    vkCmdBindPipeline(device->commandBuffer,
                      VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
                      rtPipeline.pipeline);

    vkCmdPushConstants(device->commandBuffer,
                       rtPipeline.layout,
                       VK_SHADER_STAGE_COMPUTE_BIT |
                       VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
                       VK_SHADER_STAGE_ANY_HIT_BIT_KHR |
                       VK_SHADER_STAGE_INTERSECTION_BIT_KHR |
                       VK_SHADER_STAGE_MISS_BIT_KHR |
                       VK_SHADER_STAGE_CALLABLE_BIT_KHR |
                       VK_SHADER_STAGE_RAYGEN_BIT_KHR,
                       0,
                       (int)pushConstantsSize,
                       pushConstantData);
    
    assert(device);
    assert(device->commandBuffer);

    device->vkCmdTraceRays(device->commandBuffer,
                           &sbt.rayGensTable,
                           &sbt.missProgsTable,
                           &sbt.hitGroupsTable,
                           &sbt.callablesTable,
                           dims_x, dims_y, dims_z);

    VK_CALL(EndCommandBuffer(device->commandBuffer),
            "could not end launch end buffer");
    
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



#if 1
    VkMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask
      = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask
      = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_SHADER_READ_BIT;
    cmdBufBI = {};
    cmdBufBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    VK_CALL(BeginCommandBuffer(device->commandBuffer, &cmdBufBI),
            "error beginning cmd buffer for raygen launch");
    vkCmdPipelineBarrier(device->commandBuffer,
                         /*srcStageMask*/VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         /*dstStageMask*/VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         0,
                         1, &barrier,
                         // 0, nullptr,
                         0, nullptr,
                         0,nullptr);//1, &barrier);
    // VkBufferMemoryBarrier barrier = {};
    VK_CALL(EndCommandBuffer(device->commandBuffer),
            "could not end launch end buffer");
    
    submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = NULL;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = nullptr;
    submitInfo.pWaitDstStageMask = nullptr;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &device->commandBuffer;
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.pSignalSemaphores = nullptr;

    if (firstTime)
      std::cout << "vkn: waiting for launch() to complete" << std::endl;

    VK_CALL(QueueSubmit(device->queue, 1, &submitInfo, VK_NULL_HANDLE),
            "error submitting raygen launch command(s) to queue");
    VK_CALL(QueueWaitIdle(device->queue),
            "error waiting for queue to finish raygen launch copy");
#endif
    
    if (firstTime)
      std::cout << "vkn: launch() completed" << std::endl;
}
  
    /*! frees all app-references to the given object */
  void Context::freeAllRefs(Object *object)
  {
    auto it = appUseCount.find(object);
    it->second.second = {};
    appUseCount.erase(it);
  }
    
  VKNSampler Context::createSampler(Image *_image,
                                    VKNSamplerFilterMode filterMode,
                                    VKNSamplerAddressMode addressMode_x,
                                    VKNSamplerAddressMode addressMode_y,
                                    const float *borderColorRGBA,
                                    bool unnormalizedCoords)
  {
    Image::SP image = _image->asSP<Image>();
    Sampler::SP sampler
      = std::make_shared<Sampler>(this,image,
                                  filterMode,
                                  addressMode_x,
                                  addressMode_y,
                                  borderColorRGBA,
                                  unnormalizedCoords);
    return (VKNSampler)addAppRef(sampler);
  }
  
  VKNImage Context::createImage(VKNTexelFormat texelFormat,
                                int size_x, int size_y,
                                const void *texels)
  {
    Image::SP image
      = std::make_shared<Image>(this,texelFormat,size_x,size_y,texels);
    return (VKNImage)addAppRef(image);
  }
  
} // ::vkn
