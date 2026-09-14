// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#include "vulkanite/PhysicalDevice.h"
#include "vulkanite/DriverInstance.h"
#include "vulkanite/Buffer.h"
#include "vulkanite/RayGen.h"
#include "vulkanite/Compute.h"
#include "vulkanite/MissProg.h"
#include "vulkanite/GeomType.h"
#include "vulkanite/UserGeom.h"
#include "vulkanite/Compute.h"
#include "vulkanite/Context.h"
#include "vulkanite/Triangles.h"
#include "vulkanite/InstanceAccel.h"
#include "vulkanite/Image.h"
#include "vulkanite/Sampler.h"

#define TODO(a) /* ignore*/

namespace vkn {

  template<typename T>
  typename T::SP checkGet(void *ptr)
  {
    if (!ptr) throw std::runtime_error("invalid null object handle");
    Object *o = (Object *)ptr;
    return o->asSP<T>();
  }
  
  VKN_API
  void vknBuildPrograms(VKNContext ctx)
  {
    // ((Context *)ctx)->buildPipeline();
  }

  VKN_API
  void vknBuildSBT(VKNContext ctx)
  {
    ((Context *)ctx)->buildSBT();
  }

  VKN_API
  void *vknBufferGetPointer(VKNBuffer _buffer)
  {
    assert(_buffer);
    DeviceBuffer *buffer = (DeviceBuffer *)_buffer;
    return (void *)buffer->deviceAddress();
  }

  VKN_API
  void *vknBufferMap(VKNBuffer _buffer)
  {
    DeviceBuffer *buffer = (DeviceBuffer *)_buffer;
    return (void *)buffer->map();
  }
  
  VKN_API
  void vknBufferUnmap(VKNBuffer _buffer)
  {
    assert(_buffer);
    DeviceBuffer *buffer = (DeviceBuffer *)_buffer;
    buffer->unmap();
  }

  VKN_API
  void vknTrianglesSetVertices(VKNGeom _geom,
                               uint32_t count,
                               void *d_addr,
                               size_t stride)
  {
    TrianglesGeom *geom = (TrianglesGeom *)_geom;
    geom->vertices = { d_addr, stride, count };
  }

  VKN_API
  void vknTrianglesSetIndices(VKNGeom _geom,
                              uint32_t count,
                              void *d_addr,
                              size_t stride)
  {
    TrianglesGeom *geom = (TrianglesGeom *)_geom;
    geom->indices = { d_addr, stride, count };
  }
  
  
  VKN_API
  void vknBuildPipeline(VKNContext ctx)
  {
    ((Context *)ctx)->buildPipeline();
  }
  
  VKN_API
  VKNContext vknContextCreate(VKNDevice _device,
                              size_t pushConstantsSize)
  {
    assert(_device);
    LogicalDevice::SP device
      = ((LogicalDevice *)_device)->shared_from_this();

    Context::SP context
      = std::make_shared<Context>(device,pushConstantsSize);
    context->selfReference = context;
    return (VKNContext)context.get();
  }

  VKN_API
  VKNShaderModule vknShaderModuleCreate(VKNContext ctx, 
                            const uint8_t *moduleData,
                            size_t sizeOfShaderModuleData)
  {
    std::vector<uint8_t> data = {moduleData,moduleData+sizeOfShaderModuleData};
    return (VKNShaderModule)((Context *)ctx)->createShaderModule(data);
  }
  
  VKN_API
  VKNRayGen vknRayGenCreate(VKNContext _ctx,
                            VKNShaderModule shaderModule,
                            const char *entryPoint,
                            size_t sizeOfData)
  {
    Context *ctx = (Context *)_ctx;
    try {
      return ctx->createRayGen((ShaderModule *)shaderModule,
                               entryPoint,
                               sizeOfData);
    } catch (std::exception &e) {
      std::cout << "#vkn:  exception in vknRayGenCreate(): " << e.what() << std::endl;
      return (VKNRayGen)0;
    }
  }

  VKN_API
  VKNCompute vknComputeCreate(VKNContext _ctx,
                                          VKNShaderModule shaderModule,
                                          const char *entryPoint,
                                          size_t sizeOfData)
  {
    Context *ctx = (Context *)_ctx;
    try {
      return ctx->createCompute((ShaderModule *)shaderModule,
                                entryPoint,
                                sizeOfData);
    } catch (std::exception &e) {
      std::cout << "#vkn:  exception in vknComputeCreate(): "
                << e.what() << std::endl;
      return (VKNCompute)0;
    }
  }

  VKN_API
  void vknComputeLaunch(VKNCompute _computeKernel,
                        int numBlocks_x,
                        int numBlocks_y,
                        int numBlocks_z,
                        const void *kernelData)
  {
    assert(_computeKernel);
    Compute *computeKernel = (Compute *)_computeKernel;
    computeKernel->launch(glm::ivec3{numBlocks_x,
                                     numBlocks_y,
                                     numBlocks_z},
                          kernelData);
  }
  

  VKN_API
  VKNMissProg vknMissProgCreate(VKNContext _ctx,
                            VKNShaderModule shaderModule,
                            const char *entryPoint,
                            size_t sizeOfData)
  {
    Context *ctx = (Context *)_ctx;
    try {
      return ctx->createMissProg((ShaderModule *)shaderModule,
                               entryPoint,
                               sizeOfData);
    } catch (std::exception &e) {
      std::cout << "#vkn:  exception in vknMissProgCreate(): " << e.what() << std::endl;
      return (VKNMissProg)0;
    }
  }

  VKN_API
  VKNGeomType vknGeomTypeCreate(VKNContext _ctx,
                                VKNGeomKind kind,
                                size_t sizeOfData)
  {
    Context *ctx = (Context *)_ctx;
    try {
      return ctx->createGeomType(kind,sizeOfData);
    } catch (std::exception &e) {
      std::cout << "#vkn:  exception in vknGeomTypeCreate(): " << e.what() << std::endl;
      return (VKNGeomType)0;
    }
  }

  VKN_API
  VKNGeom vknGeomCreate(VKNGeomType _gt)
  {
    GeomType *gt = (GeomType*)_gt;
    try {
      return gt->context->createGeom(gt);
    } catch (std::exception &e) {
      std::cout << "#vkn:  exception in vknGeomCreate(): " << e.what() << std::endl;
      return (VKNGeom)0;
    }
  }
  
  VKN_API
  VKNBuffer vknDeviceBufferCreate(VKNContext _ctx,
                                  size_t numBytes,
                                  const void *data)
  {
    Context *ctx = (Context *)_ctx;
    try {
      return ctx->createDeviceBuffer(numBytes,data);
    } catch (std::exception &e) {
      std::cout << "#vkn:  exception in vknDeviceBufferCreate(): " << e.what() << std::endl;
      return (VKNBuffer)0;
    }
  }

  VKN_API
  VKNImage vknImage2DCreate(VKNContext _ctx,
                            VKNTexelFormat texelFormat,
                            uint32_t size_x,
                            uint32_t size_y,
                            const void *texels)
  {
    Context *ctx = (Context *)_ctx;
    try {
      return ctx->createImage(texelFormat,size_x,size_y,texels);
    } catch (std::exception &e) {
      std::cout << "#vkn:  exception in vknImage2DCreate(): " << e.what() << std::endl;
      return (VKNImage)0;
    }
  }
  
  VKN_API
  VKNSampler vknSampler2DCreate(VKNImage _image,
                                VKNSamplerFilterMode filterMode,
                                VKNSamplerAddressMode addressMode_x,
                                VKNSamplerAddressMode addressMode_y,
                                const float *borderColorRGBA,
                                bool normalizedCoords)
  {
    assert(_image);
    Image *image = (Image *)_image;
    Context *ctx = image->context;
    try {
      return ctx->createSampler(image,
                                filterMode,
                                addressMode_x,
                                addressMode_y,
                                borderColorRGBA,
                                normalizedCoords);
    } catch (std::exception &e) {
      std::cout << "#vkn:  exception in vknSampler2DCreate(): " << e.what() << std::endl;
      return (VKNSampler)0;
    }
  }
  
  VKN_API
  void vknBufferDestroy(VKNBuffer _buffer)
  {
    assert(_buffer);
    DeviceBuffer *buffer = (DeviceBuffer *)_buffer;
    buffer->free();
    buffer->context->freeAllRefs(buffer);
  }

/*! return the resource binding index with which this image was bound
    to the respective texture1D/2D/3D[] array on the slang side of
    things */
VKN_API int
vknImageGetIndex(VKNImage image)
{
  if (!image) return -1;
  return ((Image *)image)->ID;
}

  VKN_API
  void vknRayGenSetData(VKNRayGen rg,
                        const void *dataPtr)
  {
    ((RayGen *)rg)->setData(dataPtr);
  }

  VKN_API
  void vknMissProgSetData(VKNMissProg rg,
                          const void *dataPtr)
  {
    ((MissProg *)rg)->setData(dataPtr);
  }
  
  VKN_API
  void vknGeomSetData(VKNGeom rg,
                      const void *dataPtr)
  {
    ((Geom *)rg)->setData(dataPtr);
  }

  VKN_API
  void vknRayGenLaunch2D(VKNRayGen _rg,
                         VKNMissProg _mp,
                         int nx, int ny,
                         const void *pcDataPtr)
  {
    RayGen *rg = (RayGen *)_rg;
    MissProg *mp = (MissProg *)_mp;
    rg->context->launch(rg,mp,nx,ny,1,pcDataPtr);
  }

  /*! return the resource binding index with which this sampler was bound
    to the respective texture1D/2D/3D[] array on the slang side of
    things */
  VKN_API int
  vknSamplerGetIndex(VKNSampler sampler)
  {
    if (!sampler) return -1;
    return ((Sampler *)sampler)->ID;
  }


  VKN_API
  vknTextureObject_t vknSamplerGetObject(VKNSampler _sampler)
  {
    if (!_sampler)
      return 
        /* 0 refer to the internal dummy image and dummy sampler, so
           a nullptr properly encodes ivalid image and invalid sampler */
        nullptr;
    Sampler *sampler = (Sampler*)_sampler;
    uint64_t bits
      = (uint64_t)sampler->ID
      | (((uint64_t)sampler->image->ID) << 32);
    return (vknTextureObject_t)bits;
  }

  VKN_API
  void vknGeomTypeSetIntersectProg(VKNGeomType _gt,
                                   VKNShaderModule _sm,
                                   const char *entryPoint)
  {
    assert(_gt);
    assert(_sm);
    assert(entryPoint);
    GeomType *gt = (GeomType *)_gt;
    ShaderModule *sm = (ShaderModule *)_sm;
    gt->setIS(sm,entryPoint);
  }

  VKN_API
  void vknGeomTypeSetBoundsProg(VKNGeomType _gt,
                                VKNShaderModule _sm,
                                const char *entryPoint)
  {
    assert(_gt);
    assert(_sm);
    assert(entryPoint);
    GeomType *gt = (GeomType *)_gt;
    ShaderModule *sm = (ShaderModule *)_sm;
    gt->setBoundsProg(sm,entryPoint);
  }

  VKN_API
  void vknGeomTypeSetClosestHit(VKNGeomType _gt,
                                VKNShaderModule _sm,
                                const char *entryPoint)
  {
    assert(_gt);
    assert(_sm);
    assert(entryPoint);
    GeomType *gt = (GeomType *)_gt;
    ShaderModule *sm = (ShaderModule *)_sm;
    gt->setCH(sm,entryPoint);
  }

  VKN_API
  void vknGeomTypeSetAnyHit(VKNGeomType _gt,
                            VKNShaderModule _sm,
                            const char *entryPoint)
  {
    assert(_gt);
    assert(_sm);
    assert(entryPoint);
    GeomType *gt = (GeomType *)_gt;
    ShaderModule *sm = (ShaderModule *)_sm;
    gt->setAH(sm,entryPoint);
  }

  VKN_API VKNDriverInstance vknInitialize(VKNDeviceType requestedTypes)
  {
    try {
      DriverInstance::SP di
        = std::make_shared<DriverInstance>(requestedTypes);
      // creation was a success - make sure the object doesn't die by
      // itself, and return it.
      di->createSelfReference();
      return (VKNDriverInstance)di.get();
    } catch (std::exception &e) {
      std::cout << "#vkn: could not create driver instance : " << e.what() << std::endl;
      return (VKNDriverInstance)0;
    }
  }

  VKN_API void vknTerminate(VKNDriverInstance driverInstance)
  {
    ((DriverInstance *)driverInstance)->terminate();
  }
  
  VKN_API uint32_t vknDeviceCount(VKNDriverInstance driverInstance)
  {
    assert(driverInstance);
    return ((DriverInstance *)driverInstance)->devices.size();
  }

  VKN_API const char *vknDeviceName(VKNDevice device)
  {
    assert(device);
    return ((LogicalDevice *)device)->physicalDevice->name.c_str();
  }
  
  VKN_API VKNDevice vknDeviceGet(VKNDriverInstance driverInstance,
                                 uint32_t deviceID)
  {
    assert(driverInstance);
    DriverInstance *di = (DriverInstance *)driverInstance;
    assert(deviceID < di->devices.size());
    return (VKNDevice)di->devices[deviceID].get();
  }

  VKN_API
  VKNAccel vknTrianglesAccelCreate(VKNContext _ctx,
                                   uint32_t numGeoms,
                                   VKNGeom _geoms[])
  {
    Context *ctx = (Context *)_ctx;
    return ctx->createTrianglesAccel(numGeoms,(TrianglesGeom **)_geoms);
  }

  VKN_API
  VKNAccel vknUserGeomsAccelCreate(VKNContext _ctx,
                                   uint32_t numGeoms,
                                   VKNGeom _geoms[])
  {
    Context *ctx = (Context *)_ctx;
    return ctx->createUserGeomAccel(numGeoms,(UserGeom **)_geoms);
  }
  
  VKN_API
  void vknAccelBuild(VKNAccel _accel)
   {
    ((Accel *)_accel)->build();
  }

  VKN_API
  uint64_t vknAccelGetAddress(VKNAccel _accel)
  {
    Accel *accel = (Accel *)_accel;
    return accel ? accel->deviceAddress() : 0;
  }

  
  
  VKN_API
  VKNAccel vknInstanceAccelCreate(VKNContext _ctx,
                                  uint32_t numInstances,
                                  VKNAccel _blases[])
  {
    Context *ctx = (Context *)_ctx;
    return ctx->createInstanceAccel(numInstances,(GeomAccel **)_blases);
  }

  /*! set array of instance transforms for an instance accel, where
    array of transforms is in HOST memory. transforms[] must have
    exactly as many entries as numBlases in instanceAccelCreate */
  VKN_API
  void vknSetTransformsHost(VKNAccel _accel,
                            VKNFloat4x3 *transforms)
  {
    assert(_accel);
    InstanceAccel *accel = (InstanceAccel *)_accel;
    accel->setTransformsHost(transforms);
  }

  VKN_API
  void vknGeomSetPrimCount(VKNGeom _geom, int primCount)
  {
    assert(_geom);
    UserGeom *geom = (UserGeom *)_geom;
    geom->primCount = primCount;
  }
  
}


