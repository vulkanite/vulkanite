// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#include "vulkanite/DescriptorPool.h"

namespace vkn {

  DescriptorPool::DescriptorPool(LogicalDevice::SP const device,
                                 VkDescriptorType type)
    : device(device)
  {
    poolSize.type = type;
  }
  DescriptorPool::~DescriptorPool() { destroy(); }

  void DescriptorPool::create(int count)
  {
    destroy();
      
    poolSize.descriptorCount = count;//0;
      
    VkDescriptorPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    createInfo.poolSizeCount = 1;
    createInfo.pPoolSizes = &poolSize;
    createInfo.maxSets = 1;
    createInfo.flags = /*need this?*/VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    VK_CALL(CreateDescriptorPool(device->handle,
                                 &createInfo, nullptr,
                                 &handle),
            "could not create descriptor pool");
  }

  void DescriptorPool::destroy()
  {
    if (handle == VK_NULL_HANDLE) return;
    vkDestroyDescriptorPool(device->handle, handle, nullptr);
    handle = VK_NULL_HANDLE;
  }
    
  DescriptorSetLayout::DescriptorSetLayout(LogicalDevice::SP const device)
    : device(device)
  {}
  
  void DescriptorSetLayout::create(const std::vector<VkDescriptorSetLayoutBinding> &bindings)
  {
    destroy();
    VkDescriptorSetLayoutCreateInfo createInfo = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.pBindings = bindings.data();
    createInfo.bindingCount = (int)bindings.size();
    VK_CALL(CreateDescriptorSetLayout
            (device->handle, &createInfo, nullptr, &handle),
            "could not create descriptor set layout");
    assert(handle);
  }
  
  void DescriptorSetLayout::destroy()
  {
    if (!handle) return;
      
    vkDestroyDescriptorSetLayout(device->handle,
                                 handle,
                                 nullptr);
    handle = VK_NULL_HANDLE;
  }

  DescriptorSetLayout::~DescriptorSetLayout() { destroy(); }
    
  DescriptorSet::DescriptorSet(LogicalDevice::SP const device)
    : device(device)
  {}
    
  DescriptorSet::~DescriptorSet()
  {
    free();
  }

  void DescriptorSet::free()
  {
    if (!handle) return;

    // PING; PRINT(poolHandle);
    // std::cout << "destroying descriptor set " << handle << std::endl;
    vkFreeDescriptorSets(device->handle,poolHandle,1,&handle);
    handle = 0;
  }
    
  void DescriptorSet::create(DescriptorPool &pool,
                             const DescriptorSetLayout &layout)
  {
    free();
      
    this->poolHandle = pool.handle;
    VkDescriptorSetAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = pool.handle;
    allocateInfo.pSetLayouts    = &layout.handle;
      
    allocateInfo.descriptorSetCount = 1;
    allocateInfo.pNext = nullptr;

    VK_CALL(AllocateDescriptorSets(device->handle,
                                   &allocateInfo,
                                   &handle),
            "could not allocate descriptor set");
    assert(handle);
  }
    
} // ::vkn
