// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
    (similar to owl/owl.h) */

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#ifndef _WIN32
# include <unistd.h>
#endif


#define VULKANITE_VERSION_MAJOR @VULKANITE_VERSION_MAJOR@
#define VULKANITE_VERSION_MINOR @VULKANITE_VERSION_MINOR@
#define VULKANITE_VERSION_PATCH @VULKANITE_VERSION_PATCH@

#cmakedefine01 VKN_VALIDATION_ENABLED

#ifdef __SLANG_COMPILER__
// do not include this host-file on slang side; slang can't parse this.
#else


#define VKN_API extern "C" /* todo declspec stuff */
#ifdef __cplusplus
# define VKN_IF_CPP(a) a
#else
# define VKN_IF_CPP(a) /* go away! */
#endif

/*! handle for a vulkan (driver-)'instance' that can contain one or
    more vulkan capable devices - need one per process */
typedef struct _VKNDriverInstance *VKNDriverInstance;

/*! handle to a (logical) vulkan device within a driver instance */
typedef struct _VKNDevice *VKNDevice;

/*! vulkan 'context' for a given ray tracing pipeline on a given
    vulkan device */
typedef struct _VKNContext *VKNContext;
typedef struct _VKNShaderModule *VKNShaderModule;
typedef struct _VKNRayGen *VKNRayGen;
typedef struct _VKNCompute *VKNCompute;
typedef struct _VKNMissProg *VKNMissProg;
typedef struct _VKNBuffer *VKNBuffer;
typedef struct _VKNGeomType *VKNGeomType;
typedef struct _VKNGeom *VKNGeom;
typedef struct _VKNAccel *VKNAccel;
typedef struct _VKNImage *VKNImage;
typedef struct _VKNSampler *VKNSampler;

typedef enum { VKN_GEOM_TRIANGLES, VKN_GEOM_USER } VKNGeomKind;


/*! supported formats for texels in textures */
typedef enum {
  VKN_TEXEL_FORMAT_BGRA8,
  VKN_TEXEL_FORMAT_RGBA8,
  VKN_TEXEL_FORMAT_RGBA32F,
  VKN_TEXEL_FORMAT_RGB32F,
  VKN_TEXEL_FORMAT_R8,
  VKN_TEXEL_FORMAT_R32F,
}
VKNTexelFormat;

/*! currently supported texture filter modes */
typedef enum {
  VKN_TEXTURE_NEAREST,
  VKN_SAMPLER_NEAREST=VKN_TEXTURE_NEAREST,
  VKN_TEXTURE_LINEAR,
  VKN_SAMPLER_LINEAR=VKN_TEXTURE_LINEAR
}
VKNSamplerFilterMode;

/*! currently supported texture filter modes */
typedef enum {
  VKN_SAMPLER_WRAP,
  VKN_SAMPLER_CLAMP,
  VKN_SAMPLER_BORDER,
  VKN_SAMPLER_MIRROR
}
VKNSamplerAddressMode;

typedef struct _vknTextureObject_t *vknTextureObject_t;

// typedef struct _VKNDeviceAccel *VKNDeviceAccel;
typedef struct _VKNTraversableHandle *VKNTraversableHandle;

struct VKNFloat3   { float x,y,z; };
struct VKNFloat4   { float x,y,z,w; };
struct VKNBox3     { VKNFloat3 lower, upper; };
struct VKNFloat4x3 { VKNFloat4 v[3]; };

typedef struct {
  VKNBox3 *primBounds;
  void    *geomData;
  int      primCount;
} VKNBoundsKernelArgs;

typedef enum {
  VKN_DEVICE_TYPE_DISCRETE   = 0b001,
  VKN_DEVICE_TYPE_INTEGRATED = 0b010,
  VKN_DEVICE_TYPE_VIRTUAL    = 0b100,
  VKN_DEVICE_TYPE_DEFAULT    = (VKN_DEVICE_TYPE_DISCRETE|VKN_DEVICE_TYPE_INTEGRATED)
} VKNDeviceType;

// ------------------------------------------------------------------
// creation and shutdown of vulkan driver instance
// ------------------------------------------------------------------

/*! creates a new driver instance with devices of given set of device
    type(s). if no types with these characteristics can be found, this
    will return a nullptr; otherwise, it returns a VKNDeriverInstance
    that at the end of the application sohuld be destroyed with
    vknTerminate() */
VKN_API VKNDriverInstance vknInitialize(VKNDeviceType);

/*! terminates a given driver instance, frees all devices, and all
  objects acquired within this instnace. after this call no handle
  created from this driver instance - either directly or indirectly -
  is valid ay more */
VKN_API void vknTerminate(VKNDriverInstance driverInstance);

// VKN_API void vknContextDestroy(VKNContext context);


/*! what in owl or cuda would be 'texture data' - the raw
    pixels/texels that go into a texture (to then be sampled with a
    sampler) */
VKN_API VKNImage
vknImage2DCreate(VKNContext vkn,
                 VKNTexelFormat texelFormat,
                 uint32_t size_x,
                 uint32_t size_y,
                 const void *texels);

/*! return the resource binding index with which this image was bound
    to the respective texture1D/2D/3D[] array on the slang side of
    things */
VKN_API int
vknImageGetIndex(VKNImage image);

/*! return the resource binding index with which this sampler was bound
    to the respective texture1D/2D/3D[] array on the slang side of
    things */
VKN_API int
vknSamplerGetIndex(VKNSampler sampler);

/*! returns a combined "textureobject" that describes both the sampler
    and the image it was created over. \todo replace this by
    internally creating only vulkan 'combined' image samplers, which
    do the same thing on the vulkan level */
VKN_API
vknTextureObject_t vknSamplerGetObject(VKNSampler sampler);




VKN_API VKNSampler
vknSampler2DCreate(VKNImage image,
                   VKNSamplerFilterMode filterMode VKN_IF_CPP
                   (=VKN_SAMPLER_LINEAR),
                   VKNSamplerAddressMode addressMode_x VKN_IF_CPP
                   (=VKN_SAMPLER_CLAMP),
                   VKNSamplerAddressMode addressMode_y VKN_IF_CPP
                   (=VKN_SAMPLER_CLAMP),
                   const float *borderColorRGBA VKN_IF_CPP
                   (=nullptr),
                   bool unnormalizedCoords VKN_IF_CPP
                   (=false)
                   );

// ------------------------------------------------------------------
// query of devices within a driver instance
// ------------------------------------------------------------------

/*! returns number of devices that are available within that given
    driver instance. note this only counts devices with type matching
    the deivce type(s) specified in the vknInitialize() call used to
    create the given driver instance */
VKN_API uint32_t vknDeviceCount(VKNDriverInstance driverInstance);

/*! returns a opaque device handle for a given device in the given
    driver instance. Note that the driver instance will *sort* devices
    of the desired types such that discrete devices (if requested)
    always come before integrated ones (if requested) which themselves
    come before virutal devices (if requeste), so this is NOT the same
    order as devices appear in vulkan. The handle returend by this
    call does not have to be released by the user, and is valid until
    the given driver instance gets terminated. */
VKN_API VKNDevice vknDeviceGet(VKNDriverInstance driverInstance,
                               uint32_t deviceID);

/*! returns a string representing the name of the given device. This
    pointer is valid until vknTerminate() of the instance that
    contains this device, and does not have to be - nor should it be -
    released by the user . */
VKN_API const char *vknDeviceName(VKNDevice device);
                   

// ------------------------------------------------------------------





VKN_API
VKNContext vknContextCreate(VKNDevice device, size_t sizeOfPushConstants);

VKN_API
VKNShaderModule vknShaderModuleCreate(VKNContext ctx,
                          const uint8_t *moduleData,
                          const size_t sizeOfShaderModuleData);

/*! create a new compute kernel object for the given vulkanite
    context */
VKN_API
VKNCompute vknComputeCreate(VKNContext ctx,
                            VKNShaderModule moduleItIsIn,
                            const char *entryPointName,
                            size_t sizeOfData);

VKN_API
void vknComputeLaunch(VKNCompute computeKernel,
                      int numBlocks_x,
                      int numBlocks_y,
                      int numBlocks_z,
                      const void *kernelData);

/*! create a new raygen program object for the given ray tracing
    context */
VKN_API
VKNRayGen vknRayGenCreate(VKNContext ctx,
                          VKNShaderModule moduleItIsIn,
                          const char *entryPointName,
                          size_t sizeOfData);

VKN_API
VKNMissProg vknMissProgCreate(VKNContext ctx,
                              VKNShaderModule moduleItIsIn,
                              const char *entryPointName,
                              size_t sizeOfData);

VKN_API
void vknRayGenSetData(VKNRayGen rg,
                      const void *dataPtr);
VKN_API
void vknMissProgSetData(VKNMissProg rg,
                      const void *dataPtr);
VKN_API
void vknGeomSetData(VKNGeom rg,
                      const void *dataPtr);

/*! (Only) for user geometry: specifies number of primitives in this
    geometry */
VKN_API
void vknGeomSetPrimCount(VKNGeom userGeom, int primCount);


VKN_API
void vknRayGenLaunch2D(VKNRayGen rg,
                       VKNMissProg mp,
                       int x, int y,
                       /*! pointer to push constant data. size of that
                         data *must* be same as specified during
                         contextcreate */
                       const void *pcDataPtr);


VKN_API
VKNGeomType vknGeomTypeCreate(VKNContext ctx,
                              VKNGeomKind kind,
                              size_t sizeOfData);

VKN_API
VKNGeom vknGeomCreate(VKNGeomType gt);

VKN_API
void vknTrianglesSetVertices(VKNGeom geom,
                             uint32_t count,
                             void *d_addr,
                             size_t stride);

VKN_API
void vknTrianglesSetIndices(VKNGeom geom,
                            uint32_t count,
                            void *d_addr,
                            size_t stride);

VKN_API
void vknGeomTypeSetClosestHit(VKNGeomType gt,
                              VKNShaderModule shaderModule,
                              const char *entryPoint);

VKN_API
void vknGeomTypeSetAnyHit(VKNGeomType gt,
                          VKNShaderModule shaderModule,
                          const char *entryPoint);

VKN_API
void vknGeomTypeSetBoundsProg(VKNGeomType gt,
                              VKNShaderModule shaderModule,
                              const char *entryPoint);

VKN_API
void vknGeomTypeSetIntersectProg(VKNGeomType gt,
                                 VKNShaderModule shaderModule,
                                 const char *entryPoint);

VKN_API
VKNBuffer vknDeviceBufferCreate(VKNContext ctx,
                                size_t numBytes,
                                // may be null:
                                const void *initData);

VKN_API
void *vknBufferGetPointer(VKNBuffer buffer);

VKN_API
void *vknBufferMap(VKNBuffer buffer);

VKN_API
void vknBufferUnmap(VKNBuffer buffer);

VKN_API
void vknBufferDestroy(VKNBuffer buffer);

VKN_API
void vknBuildPrograms(VKNContext ctx);

VKN_API
void vknBuildPipeline(VKNContext ctx);

VKN_API
void vknBuildSBT(VKNContext ctx);

VKN_API
uint64_t vknAccelGetAddress(VKNAccel accel);


inline 
VKNTraversableHandle vknAccelGetTraversableHandle(VKNAccel accel)
{ return (VKNTraversableHandle)vknAccelGetAddress(accel); }

VKN_API
VKNAccel vknTrianglesAccelCreate(VKNContext _ctx,
                                 uint32_t numGeoms,
                                 VKNGeom geoms[]);
VKN_API
VKNAccel vknUserGeomsAccelCreate(VKNContext _ctx,
                                 uint32_t numGeoms,
                                 VKNGeom geoms[]);

VKN_API
void vknAccelBuild(VKNAccel _accel);
  
VKN_API
VKNAccel vknInstanceAccelCreate(VKNContext _ctx,
                                uint32_t numBlases,
                                VKNAccel blases[]);

/*! set array of instance transforms for an instance accel, where
    array of transforms is in HOST memory. transforms[] must have
    exactly as many entries as numBlases in instanceAccelCreate */
VKN_API
void vknSetTransformsHost(VKNAccel _accel,
                          VKNFloat4x3 *transforms);

#ifdef WIN32
# define VKN_TERMINAL_RED ""
# define VKN_TERMINAL_GREEN ""
# define VKN_TERMINAL_LIGHT_GREEN ""
# define VKN_TERMINAL_YELLOW ""
# define VKN_TERMINAL_BLUE ""
# define VKN_TERMINAL_LIGHT_BLUE ""
# define VKN_TERMINAL_RESET ""
# define VKN_TERMINAL_DEFAULT VKN_TERMINAL_RESET
# define VKN_TERMINAL_BOLD ""

# define VKN_TERMINAL_MAGENTA ""
# define VKN_TERMINAL_LIGHT_MAGENTA ""
# define VKN_TERMINAL_CYAN ""
# define VKN_TERMINAL_LIGHT_RED ""
#else
# define VKN_TERMINAL_RED "\033[0;31m"
# define VKN_TERMINAL_GREEN "\033[0;32m"
# define VKN_TERMINAL_LIGHT_GREEN "\033[1;32m"
# define VKN_TERMINAL_YELLOW "\033[1;33m"
# define VKN_TERMINAL_BLUE "\033[0;34m"
# define VKN_TERMINAL_LIGHT_BLUE "\033[1;34m"
# define VKN_TERMINAL_RESET "\033[0m"
# define VKN_TERMINAL_DEFAULT VKN_TERMINAL_RESET
# define VKN_TERMINAL_BOLD "\033[1;1m"

# define VKN_TERMINAL_MAGENTA "\e[35m"
# define VKN_TERMINAL_LIGHT_MAGENTA "\e[95m"
# define VKN_TERMINAL_CYAN "\e[36m"
# define VKN_TERMINAL_LIGHT_RED "\033[1;31m"
#endif


#endif
