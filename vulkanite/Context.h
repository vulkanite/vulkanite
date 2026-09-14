// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#include "vulkanite/LogicalDevice.h"
#include "vulkanite/RTPipeline.h"
#include "vulkanite/Compute.h"
#include "vulkanite/Buffer.h"
#include "vulkanite/DescriptorPool.h"
#include "vulkanite/RangeAllocator.h"

#define LOG_CONSTRUCTION(class,ptr) /* nothing */
#define LOG_DESTRUCTION(class,ptr) /* nothing */

namespace vkn {

  struct Context;
  struct RayGen;
  struct MissProg;
  struct GeomType;
  struct Compute;
  struct DeviceBuffer;
  struct GeomAccel;
  struct Image;
  struct Sampler;
  
  struct StorageBuffer;
  struct UniformBuffer;
  struct TrianglesGeom;
  struct UserGeom;

  struct IDPool {
    struct Object {
      Object(IDPool *pool) : ID(pool->alloc()) {}
      int const ID;
    };
    void release(int ID) {
      assert(ID >= 0);
      assert(ID < maxUsed);
      freeIDs.push(ID);
    }
    int alloc() {
      if (freeIDs.empty()) { return maxUsed++; }
      int ID = freeIDs.top();
      freeIDs.pop();
      return ID;
    }
    inline bool inRange(int ID) { return ID >= 0 && ID < maxUsed; }
    std::stack<int>  freeIDs;
    int maxUsed = 0;
  };

  
  struct Context : public std::enable_shared_from_this<Context> {
    typedef std::shared_ptr<Context> SP;
    
    Context(LogicalDevice::SP device,
            size_t pushConstantsSize);
    ~Context();
    
    void buildPipeline();
    void buildSBT();
    // void launch(int nx, int ny, int nz, const void *pcData);
    
    LogicalDevice::SP       device;
    size_t            const pushConstantsSize;    
    RTPipeline              rtPipeline;
    Context::SP             selfReference;
    
    struct {
      VkStridedDeviceAddressRegionKHR rayGensTable{};
      VkStridedDeviceAddressRegionKHR hitGroupsTable{};
      VkStridedDeviceAddressRegionKHR missProgsTable{};
      VkStridedDeviceAddressRegionKHR callablesTable{};
    } sbt;

    DeviceBuffer::SP rayGenRecordsBuffer;
    DeviceBuffer::SP missProgRecordsBuffer;
    DeviceBuffer::SP hitGroupRecordsBuffer;
    
    void rebuildDescriptors();
    void launch(RayGen *rg,
                MissProg *mp,
                int nx, int ny, int nz,
                const void *pcData);
    ShaderModule *createShaderModule(const std::vector<uint8_t> &moduleData);

    VKNBuffer createDeviceBuffer(size_t numBytes,
                                 const void *initData);
    VKNRayGen createRayGen(ShaderModule *shaderModule,
                           const char *entryPoint,
                           size_t sbtSize);
    VKNCompute createCompute(ShaderModule *shaderModule,
                             const char *entryPoint,
                             size_t sbtSize);
    VKNMissProg createMissProg(ShaderModule *shaderModule,
                               const char *entryPoint,
                               size_t sbtSize);
    VKNGeomType createGeomType(VKNGeomKind kind,
                               size_t sbtSize);
    VKNGeom createGeom(GeomType *gt);
    VKNAccel createTrianglesAccel(int numGeoms,
                                  TrianglesGeom **geoms);
    VKNAccel createUserGeomAccel(int numGeoms,
                                 UserGeom **geoms);
    VKNAccel createInstanceAccel(int numInstances,
                                 GeomAccel **accels);
    VKNSampler createSampler(Image *image,
                             VKNSamplerFilterMode filterMode,
                             VKNSamplerAddressMode addressMode_x,
                             VKNSamplerAddressMode addressMode_y,
                             const float *borderColorRGBA,
                             bool normalizedCoords);
    VKNImage createImage(VKNTexelFormat texelFormat,
                         int size_x, int size_y,
                         const void *texels);

    void terminate();

    RangeAllocator sbtRangeAllocator;
    
    template<typename T>
    T *addAppRef(const std::shared_ptr<T> &object);

    /*! frees all app-references to the given object */
    void freeAllRefs(Object *object);
    
    std::map<Object *,std::pair<int,Object::SP>> appUseCount;

    // some sampler to force population non-empty sampler and image
    // lists; this will also force these to have image ID 0 and
    // sampler ID 0, respecgively, which means we can use the value
    // '0' to indicate 'invalid image/sampler
    std::shared_ptr<Sampler> defaultSampler;
    std::shared_ptr<Image>   defaultImage;
    
    DescriptorPool      uniformBuffersPool;
    DescriptorSetLayout uniformBuffersLayout;
    DescriptorSet       uniformBuffersDS;
  };
  
} // :: vkn
