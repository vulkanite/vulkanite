// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "vulkanite/Image.h"

namespace vkn {
  
  struct Sampler : public Object {
    typedef std::shared_ptr<Sampler> SP;
    Sampler(Context *context,
            Image::SP image,
            VKNSamplerFilterMode filterMode,
            VKNSamplerAddressMode addressMode_x,
            VKNSamplerAddressMode addressMode_y,
            const float *borderColorRGBA,
            bool unnormalizedCoords);
    ~Sampler() override;
    
    std::string toString() override { return "Sampler"; }
    VkDescriptorImageInfo getDescriptorInfo()
    {
      VkDescriptorImageInfo info = {};
      assert(handle);
      info.sampler = handle;
      info.imageView = VK_NULL_HANDLE;
      return info;
    }
    
    VkSampler handle = 0;
    Image::SP image;
    const int ID;
  };

} // ::vkn

