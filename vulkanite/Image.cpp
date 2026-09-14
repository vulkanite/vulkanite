// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#include "vulkanite/Image.h"
#include "vulkanite/LogicalDevice.h"
#include "vulkanite/Context.h"

namespace vkn {

  VkFormat toVulkan(VKNTexelFormat texelFormat)
  {
    switch(texelFormat) {
    case VKN_TEXEL_FORMAT_BGRA8:
      return VK_FORMAT_B8G8R8A8_SRGB;
    case VKN_TEXEL_FORMAT_RGBA8:
      return VK_FORMAT_R8G8B8A8_SRGB;
    case VKN_TEXEL_FORMAT_RGBA32F:
      return VK_FORMAT_R32G32B32A32_SFLOAT;
    case VKN_TEXEL_FORMAT_RGB32F:
      return VK_FORMAT_R32G32B32_SFLOAT;
    default:
      PING;
      throw std::runtime_error
        ("unrecognized texel format #"+std::to_string((int)texelFormat));
    }
  }

  size_t sizeOf(VKNTexelFormat texelFormat)
  {
    switch(texelFormat) {
    case VKN_TEXEL_FORMAT_RGBA32F:
      return 16;
    case VKN_TEXEL_FORMAT_RGB32F:
      return 12;
    case VKN_TEXEL_FORMAT_RGBA8:
    case VKN_TEXEL_FORMAT_BGRA8:
      return 4;
    default:
      PING;
      throw std::runtime_error
        ("unrecognized texel format #"+std::to_string((int)texelFormat));
    }
  }

  Image::Image(Context *context,
               VKNTexelFormat texelFormat,
               int nx, int ny,
               const void *texels)
    : Object(context),
      ID(context->rtPipeline.image2Ds.insert(this))
  {
    auto device = context->device;

    auto aspectFlagBits = VK_IMAGE_ASPECT_COLOR_BIT;
    uint32_t mipLevels = 1;
    
    VkImageCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ci.imageType = VK_IMAGE_TYPE_2D;
    ci.extent.width = nx;
    ci.extent.height = ny;
    ci.extent.depth = 1;
    ci.mipLevels = mipLevels;
    ci.arrayLayers = 1;
    ci.format = toVulkan(texelFormat);
    // ci.tiling = VK_IMAGE_TILING_OPTIMAL;
    ci.tiling = VK_IMAGE_TILING_LINEAR;
    ci.initialLayout
      // = VK_IMAGE_LAYOUT_UNDEFINED;
      = VK_IMAGE_LAYOUT_PREINITIALIZED; // need this if we upload pixels!?
    // ci.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    ci.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    // ci.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    ci.samples = VK_SAMPLE_COUNT_1_BIT;
    ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VK_CHECK(vkCreateImage(device->handle,
                        &ci,
                        nullptr,
                        &this->handle),
            "could not create VkImage");
    assert(this->handle != VK_NULL_HANDLE);

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(device->handle,
                                 this->handle, &memReqs);
    imageMemory
      = device->allocateMemory(memReqs,
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
                               |
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                               );

    VkImageSubresource subresource{};
    subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    VkSubresourceLayout layout{};
    vkGetImageSubresourceLayout(device->handle, this->handle, &subresource, &layout);

    const size_t texelSize = sizeOf(texelFormat);
    const size_t rowSize = nx * texelSize;
    // std::cout << "mapping image memory" << std::endl;
    void *mapped = 0;
    VK_CHECK(vkMapMemory(device->handle, imageMemory, 0, memReqs.size, 0, &mapped),
            "could not map image memory");
    assert(mapped);

    uint8_t *dst = (uint8_t *)mapped + layout.offset;
    const uint8_t *src = (const uint8_t *)texels;
    for (int y = 0; y < ny; ++y)
      memcpy(dst + y * layout.rowPitch, src + y * rowSize, rowSize);
    vkUnmapMemory(device->handle, imageMemory);

    VK_CHECK(vkBindImageMemory(device->handle, this->handle,
                               imageMemory, 0),
             "error binding image memory");

    {
      VkResult err;
      VkCommandBufferBeginInfo cmdBufInfo{};
      cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      VK_CHECK(vkBeginCommandBuffer(device->commandBuffer, &cmdBufInfo),
               "could not begin command buffer for setting image layout");
      
      VkImageSubresourceRange subresourceRange
        = {uint32_t(aspectFlagBits), 0, mipLevels, 0, 1};
      VkImageMemoryBarrier imageMemoryBarrier{};
      imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      
      imageMemoryBarrier.oldLayout
        // = VK_IMAGE_LAYOUT_UNDEFINED;
        = VK_IMAGE_LAYOUT_PREINITIALIZED;
      imageMemoryBarrier.newLayout
        // = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        // = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        // = VK_IMAGE_LAYOUT_PREINITIALIZED;
        = VK_IMAGE_LAYOUT_GENERAL;
      this->layout = VK_IMAGE_LAYOUT_GENERAL;
      imageMemoryBarrier.image = this->handle;
      imageMemoryBarrier.subresourceRange = subresourceRange;
      imageMemoryBarrier.srcAccessMask
        // = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        = VK_ACCESS_TRANSFER_WRITE_BIT;
      imageMemoryBarrier.dstAccessMask
        = VK_ACCESS_SHADER_READ_BIT;
      vkCmdPipelineBarrier(device->commandBuffer,
                           /*srcStageMask*/VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                           /*dstStageMask*/VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                           0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
      

      // layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

      VK_CHECK(vkEndCommandBuffer(device->commandBuffer),
               "error ending command buffer for setting image layout");

      VkSubmitInfo submitInfo{};
      submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
      submitInfo.pNext = NULL;
      submitInfo.waitSemaphoreCount = 0;
      submitInfo.pWaitSemaphores = nullptr;     //&acquireImageSemaphoreHandleList[currentFrame];
      submitInfo.pWaitDstStageMask = nullptr;   //&pipelineStageFlags;
      submitInfo.commandBufferCount = 1;
      submitInfo.pCommandBuffers = &device->commandBuffer;
      submitInfo.signalSemaphoreCount = 0;
      submitInfo.pSignalSemaphores = nullptr;   //&writeImageSemaphoreHandleList[currentImageIndex]};

      VK_CHECK(vkQueueSubmit(device->queue, 1, &submitInfo, VK_NULL_HANDLE),
              "error submitting queue for setting image layout");
      VK_CHECK(vkQueueWaitIdle(device->queue),
              "error waiting for queue setting image layout");
      // std::cout << "image layout set" << std::endl;
    }

    // Now we need an image view
    imageView = VK_NULL_HANDLE;
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = this->handle;
    viewInfo.viewType = (VkImageViewType)ci.imageType;
    viewInfo.format = ci.format;
    viewInfo.subresourceRange.aspectMask = aspectFlagBits;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    
    VK_CHECK(vkCreateImageView(device->handle, &viewInfo, nullptr, &imageView),
             "could not create image view");
  }
  
  Image::~Image()
  {
    vkDestroyImageView(device->handle,imageView, nullptr);
    vkDestroyImage(device->handle,this->handle, nullptr);
    this->handle = VK_NULL_HANDLE;
    device->freeMemory(imageMemory);

    context->rtPipeline.image2Ds.remove(this->ID);
  }

  VkDescriptorImageInfo Image::getDescriptorInfo()
  {
    VkDescriptorImageInfo info = {};
    info.imageView = imageView;
    info.imageLayout = layout;
    return info;
  }
  
  
}

