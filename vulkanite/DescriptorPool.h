// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "vulkanite/LogicalDevice.h"

namespace vkn {

  struct DescriptorPool {
    DescriptorPool(LogicalDevice::SP const device,
                   VkDescriptorType type);
    ~DescriptorPool();

    void create(int count);
    void destroy();
    
    VkDescriptorPool     handle      = VK_NULL_HANDLE;
    VkDescriptorPoolSize poolSize    = {};
    LogicalDevice::SP const device;
  };

  struct DescriptorSetLayout {
    DescriptorSetLayout(LogicalDevice::SP const device);
    void create(const std::vector<VkDescriptorSetLayoutBinding> &bindings);
    void destroy();
    ~DescriptorSetLayout();
    
    VkDescriptorSetLayout handle = VK_NULL_HANDLE;
    LogicalDevice::SP const device;
  };
  
  struct DescriptorSet {
    DescriptorSet(LogicalDevice::SP const device);
    ~DescriptorSet();
    
    void free();
    void create(DescriptorPool &pool,
                const DescriptorSetLayout &layout);
    
    VkDescriptorPool poolHandle = 0;
    VkDescriptorSet       handle = VK_NULL_HANDLE;
    LogicalDevice::SP const device;
  };
  
} // ::vkn
