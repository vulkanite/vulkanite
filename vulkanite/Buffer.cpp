// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#define VMA_IMPLEMENTATION

#include "vulkanite/Buffer.h"
#include "vulkanite/LogicalDevice.h"
#include "vulkanite/Context.h"

namespace vkb {
  namespace initializers {
    inline VkFramebufferCreateInfo framebuffer_create_info()
    {
      VkFramebufferCreateInfo framebuffer_create_info{};
      framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      return framebuffer_create_info;
    }

    inline VkBufferCreateInfo buffer_create_info(
                                                 VkBufferUsageFlags usage,
                                                 VkDeviceSize       size)
    {
      VkBufferCreateInfo buffer_create_info{};
      buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      buffer_create_info.usage = usage;
      buffer_create_info.size  = size;
      return buffer_create_info;
    }


    inline VkMemoryAllocateInfo memory_allocate_info()
    {
      VkMemoryAllocateInfo memory_allocation{};
      memory_allocation.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      return memory_allocation;
    }
    
  }
}

namespace vkn {

  DeviceBuffer::DeviceBuffer(Context *context,
                             size_t sizeInBytes,
                             size_t align)
    : Object(context),
      device(context->device),
      sizeInBytes(sizeInBytes)
  {
    LOG_CONSTRUCTION(DeviceBuffer,this);
    
    alignment = std::max(alignment,align);
    assert(context);
    assert(device);
    
    VkBufferUsageFlags usageFlags = 
      // means we can get this buffer's address with vkGetBufferDeviceAddress
      VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
      // means we can use this buffer to transfer into another
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
      // means we can use this buffer to receive data transferred from another
      VK_BUFFER_USAGE_TRANSFER_DST_BIT |
      // means we can use this buffer as a storage buffer resource
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |

      // can use this as a *build input* to an accel struct build
      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
      // can use this to *store* an accel struct
      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
      
      VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR|
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.usage = usageFlags;
    bufferCreateInfo.size = sizeInBytes;
      
    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;

    VK_CHECK(vmaCreateBufferWithAlignment(device->allocator.handle,
                                          &bufferCreateInfo,
                                          &allocInfo,
                                          alignment,
                                          &handle,
                                          &allocation,
                                          nullptr),
             "could not allocate device buffer");

    VkBufferDeviceAddressInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO_KHR;
    info.buffer = handle;
    devPtr = device->vkGetBufferDeviceAddress(device->handle, &info);
  }
    
  DeviceBuffer::~DeviceBuffer()
  {
    free();
    
    LOG_DESTRUCTION(DeviceBuffer,this);
  }

  /*! free whatever device data we are currently holding. will not
    destroy the buffer class itself, but free all device data */
  void DeviceBuffer::free()
  {
    if (handle == VK_NULL_HANDLE) return;

    vmaDestroyBuffer(device->allocator.handle, handle, allocation);
    // vkDestroyBuffer(device->handle, handle, nullptr);
    handle = VK_NULL_HANDLE;
  }
    

  void *DeviceBuffer::map()
  {
    assert(mapped == nullptr && "buffer already mapped");
    vmaInvalidateAllocation(device->allocator.handle, allocation, 0, VK_WHOLE_SIZE);
    vmaMapMemory(device->allocator.handle, allocation, &mapped);
    assert(mapped != nullptr && "could not map buffer");
    return mapped;
  }

  void DeviceBuffer::upload(const void *hostData)
  {
    void *mapped = map();
    memcpy(mapped,hostData,sizeInBytes);
    unmap();
  }
  
  void DeviceBuffer::unmap()
  {
    assert(mapped != nullptr && "unmapping buffer that isn't mapped!?");
    vmaFlushAllocation(device->allocator.handle, allocation, 0, VK_WHOLE_SIZE);
    vmaUnmapMemory(device->allocator.handle, allocation);
    mapped = nullptr;
  }

  void DeviceBuffer::copy(DeviceBuffer *dst, DeviceBuffer *src)
  {
    assert(src);
    assert(dst);
    assert(dst->sizeInBytes == src->sizeInBytes);

    assert(dst->device == src->device);
    auto device = src->device;
    assert(device);
      
    auto &commandBuffer = device->commandBuffer;
    assert(commandBuffer);

    VkCommandBufferBeginInfo cmdBufInfo{};
    cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    VK_CALL(BeginCommandBuffer(commandBuffer, &cmdBufInfo),
            "error beginning cmd buffer for buffer copy");
      
    VkBufferCopy region;
    region.srcOffset = 0;
    region.dstOffset = 0;
    region.size = src->sizeInBytes;
      
    vkCmdCopyBuffer(commandBuffer,
                    src->handle,
                    dst->handle, 1, &region);

    VK_CALL(EndCommandBuffer(commandBuffer),
            "error ending cmd buffer for buffer copy");

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = NULL;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = nullptr;
    submitInfo.pWaitDstStageMask = nullptr;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.pSignalSemaphores = nullptr;

    VK_CALL(QueueSubmit(device->queue, 1, &submitInfo, VK_NULL_HANDLE),
            "error submitting buffer copy to queue");
    VK_CALL(QueueWaitIdle(device->queue),
            "error waiting for queue to finish buffer copy");

  }

} // ::vkn

