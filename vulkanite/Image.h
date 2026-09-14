// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "vulkanite/LogicalDevice.h"
#include "vulkanite/Object.h"

namespace vkn {
    
  struct Image : public Object {
    typedef std::shared_ptr<Image> SP;
    Image(Context *context,
          VKNTexelFormat texelFormat,
          int nx, int ny,
          const void *texels);
    ~Image() override;

    std::string toString() override { return "Image"; }
    VkDescriptorImageInfo getDescriptorInfo();
    
    VkImage handle = 0;
    int size[2] = {0,0};
    VkImageView imageView = VK_NULL_HANDLE;
    VkImageLayout layout = VK_IMAGE_LAYOUT_GENERAL;
    VkDeviceMemory imageMemory = 0;
    
    VKNTexelFormat texelFormat;
    const int ID;
  };

} // ::vkn

