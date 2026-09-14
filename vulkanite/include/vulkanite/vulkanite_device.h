// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#pragma once

[[vk::binding(0, 1)]] SamplerState samplers[];
[[vk::binding(0, 2)]] Texture2D texture2Ds[];

// struct vknTextureObject_t { uint16_t samplerID; uint16_t imageID; }
struct _vknTextureObject_t;
typedef _vknTextureObject_t *vknTextureObject_t;

struct box3 {
  float3 lower, upper;
};

namespace vkn {
  // typedef float2 vec2f;
  // typedef float3 vec3f;
  // typedef float4 vec4f;
  // typedef int2   vec2i;
  // typedef int3   vec3i;
  // typedef box3   box3f;

  /*! HAS to match VKNBoudnsKernelArgs in vulkanite.h */
  struct BoundsKernelArgs{
    box3 *primBounds;
    void    *geomData;
    int      primCount;
  };

  
  // Until Slang PR here is merged in, use these accessors to avoid issues 
  // with Mesa for Intel Arc and for RDNA2/3/4 on RADV 
  // https://github.com/shader-slang/slang/pull/10656
  [ForceInline]
  uint __NonUniformTex2DIndex(uint idx)
  {
    __target_switch
    {
      case spirv:
        const uint idxCopy = __copyObject(idx);
        spirv_asm { OpDecorate $idxCopy NonUniform; };
        return NonUniformResourceIndex(idxCopy);
      default:
        return idx;
    }
  }

  [ForceInline]
  uint __NonUniformSamplerIndex(uint idx)
  {
    __target_switch
    {
      case spirv:
        const uint idxCopy = __copyObject(idx);
        spirv_asm { OpDecorate $idxCopy NonUniform; };
        return NonUniformResourceIndex(idxCopy);
      default:
        return idx;
    }
  }

  float4 tex2D(vknTextureObject_t to, float2 tc)
  {
    uint64_t bits = (uint64_t)to;
    uint imageID = (uint)(bits>>32);
    uint samplerID = (uint)bits;
    // Sampler sampler = pc.globals->samplers[combinedSamplerID];
    Texture2D    _texture = texture2Ds[__NonUniformTex2DIndex(imageID)];
    SamplerState _sampler = samplers[__NonUniformSamplerIndex(samplerID)];
    float4       v        = _texture.SampleLevel(_sampler, tc, 0.f);
    return v;
  }
    
  struct Ray {
    float3 origin;
    float  tMin;
    float3 direction;
    float  tMax;
  };

  inline RayDesc make_RayDesc(Ray ray)
  {
    RayDesc rayDesc = {};
    rayDesc.Origin = ray.origin;
    rayDesc.Direction = ray.direction;
    rayDesc.TMin = ray.tMin;
    rayDesc.TMax = ray.tMax;
    return rayDesc;
  }

  RaytracingAccelerationStructure to_vulkan(VKNTraversableHandle address)
  {
    return spirv_asm {
    result: $$RaytracingAccelerationStructure = OpConvertUToAccelerationStructureKHR $address
    };
  }

  inline void traceRay<PRD>(VKNTraversableHandle world,
                            Ray ray,
                            inout PRD prd)
  {
    RayDesc                         rd       = make_RayDesc(ray);
    RaytracingAccelerationStructure vknWorld = to_vulkan(world);
    TraceRay(vknWorld,
             0,//RAY_FLAG_FORCE_OPAQUE,   // ray flags
             0xff,                    // instance inclusion mask
             0,                       // ray type
             1,                       // number of ray types
             0,                       // miss type
             rd,
             prd);
  }
  
  inline uint32_t
  make_8bit(const float f) {
    return min(255, max(0, int(f * 256.f)));
  }

  inline uint32_t
  make_rgba(float3 color) {
    // float gamma = 2.2;
    // color = pow(color, float3(1.0f / gamma, 1.0f / gamma, 1.0f / gamma));
    return
      (make_8bit(color.x) << 0) +
      (make_8bit(color.y) << 8) +
      (make_8bit(color.z) << 16) +
      (0xffU << 24);
  }

  inline uint32_t
  make_rgba(float4 color) {
    // float gamma = 2.2;
    // color = pow(color, float3(1.0f / gamma, 1.0f / gamma, 1.0f / gamma));
    return
      (make_8bit(color.x) << 0) +
      (make_8bit(color.y) << 8) +
      (make_8bit(color.z) << 16) +
      (make_8bit(color.w) << 24);
  }

  inline uint32_t
  make_bgra(float3 color) {
    // float gamma = 2.2;
    // color = pow(color, float3(1.0f / gamma, 1.0f / gamma, 1.0f / gamma));
    return
      (make_8bit(color.z) << 0) +
      (make_8bit(color.y) << 8) +
      (make_8bit(color.x) << 16) +
      (0xffU << 24);
  }

  inline uint32_t
  make_bgra(float4 color) {
    // float gamma = 2.2;
    // color = pow(color, float3(1.0f / gamma, 1.0f / gamma, 1.0f / gamma));
    return
      (make_8bit(color.z) << 0) +
      (make_8bit(color.y) << 8) +
      (make_8bit(color.x) << 16) +
      (make_8bit(color.w) << 24);
  }

}

#ifdef __SLANG_COMPILER__
typedef uint64_t VKNTraversableHandle;
#else
typedef struct _VKNDeviceAccel{} *VKNTraversableHandle;
#endif
