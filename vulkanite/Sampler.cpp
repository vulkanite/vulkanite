// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#include "vulkanite/Sampler.h"
#include "vulkanite/LogicalDevice.h"
#include "vulkanite/Context.h"

namespace vkn {

  VkSamplerAddressMode toVulkan(VKNSamplerAddressMode mode)
  {
    switch(mode) {
    case VKN_SAMPLER_WRAP:   return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    case VKN_SAMPLER_CLAMP:  return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    case VKN_SAMPLER_BORDER: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    case VKN_SAMPLER_MIRROR: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    default: assert(0);
    };
    assert(0);
    return VK_SAMPLER_ADDRESS_MODE_REPEAT;
  }
  
  VkFilter toVulkan(VKNSamplerFilterMode ours)
  {
    switch (ours) {
    case VKN_SAMPLER_NEAREST  : return VK_FILTER_NEAREST;
    case VK_FILTER_LINEAR     : return VK_FILTER_LINEAR;
    default: VK_FILTER_LINEAR : return VK_FILTER_LINEAR;
    }
  }

  Sampler::Sampler(Context *context,
            Image::SP image,
            VKNSamplerFilterMode filterMode,
            VKNSamplerAddressMode addressMode_x,
            VKNSamplerAddressMode addressMode_y,
            const float *borderColorRGBA,
            bool unnormalizedCoords)
    : Object(context),
      image(image),
      ID(context->rtPipeline.samplers.insert(this))
  {
    auto device = context->device;
    
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = toVulkan(filterMode);
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = toVulkan(addressMode_x);
    samplerInfo.addressModeV = toVulkan(addressMode_y);
    samplerInfo.addressModeW = toVulkan(addressMode_x);
    samplerInfo.anisotropyEnable = VK_FALSE;

    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
    // if (borderColorRGBA)
    //   memcpy(samplerInfo.borderColor,borderColorRGBA,
    //          4*sizeof(float));
    // else
    //   memset(samplerInfo.borderColor,0,4*sizeof(float));
    

    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    VK_CALL(CreateSampler(device->handle, &samplerInfo,
                            nullptr, &this->handle),
            "could not create sampler");
  }
  
  Sampler::~Sampler()
  {
    vkDestroySampler(device->handle,this->handle, nullptr);
    this->handle = VK_NULL_HANDLE;
    context->rtPipeline.samplers.remove(this->ID);
  }
  
}

