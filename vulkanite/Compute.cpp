// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#include "vulkanite/Compute.h"
#include "vulkanite/Context.h"

namespace vkn {

  Compute::Compute(ShaderModule *shaderModule,
                   const std::string &entryPoint,
                   const size_t size)
    : ShaderProgram(shaderModule,entryPoint,
                    VK_SHADER_STAGE_COMPUTE_BIT
                    // VK_SHADER_STAGE_RAYGEN_BIT_KHR
                    ),
      size(size)
  {
    buildPipeline();
  }

  Compute::~Compute()
  {
    std::cout << "#vkn: ComputePipeline is dying" << std::endl;
    assert(device);
    std::cout << " - killing pipeline " << (int*)pipeline << std::endl;
    vkDestroyPipeline(device->getHandle(),pipeline,nullptr);
    std::cout << " - killing pipeline layout " << (int*)pipelineLayout << std::endl;
    vkDestroyPipelineLayout(device->handle, pipelineLayout, nullptr);
    std::cout << "... done " << std::endl;
  }

  void Compute::launch(const glm::ivec3 numBlocks,
                       const void *pArgs)
  {
    assert(pipelineLayout);
      
    VkCommandBufferBeginInfo cmdBufBI{};
    cmdBufBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    VK_CALL(BeginCommandBuffer(device->commandBuffer, &cmdBufBI),
            "error beginning cmd buffer for raygen launch");
    uint32_t offset = (uint32_t)0;

    if (context->uniformBuffersDS.handle == 0) {
      std::cout << "NULL uniform buffer desc ... rebuilding descriptors...." << std::endl;
      context->rebuildDescriptors();
    }
    
    auto &rtPipeline = context->rtPipeline;
    if (context->uniformBuffersDS.handle == 0) {
      context->rebuildDescriptors();
    }
    std::vector<VkDescriptorSet> descriptorSets = {
      context->uniformBuffersDS.handle,
      rtPipeline.samplers.descriptorSet.handle,
      rtPipeline.image2Ds.descriptorSet.handle
    };
    vkCmdBindDescriptorSets(device->commandBuffer,
                            VK_PIPELINE_BIND_POINT_COMPUTE,
                            rtPipeline.layout,
                            0,
                            (uint32_t)descriptorSets.size(),
                            descriptorSets.data(),
                            0,nullptr);
    vkCmdBindPipeline(device->commandBuffer,
                      VK_PIPELINE_BIND_POINT_COMPUTE,
                      pipeline);
      
    static float dummy = 0.f;
    vkCmdPushConstants(device->commandBuffer,
                       pipelineLayout,
                       VK_SHADER_STAGE_COMPUTE_BIT,
                       0,
                       size?size:sizeof(float),
                       pArgs?pArgs:(const void *)&dummy
                       ); 
    vkCmdDispatch(device->commandBuffer,
                  uint32_t(numBlocks[0]),
                  uint32_t(numBlocks[1]),
                  uint32_t(numBlocks[2]));

    VK_CALL(EndCommandBuffer(device->commandBuffer),
            "could not end launch end buffer");
    
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = NULL;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = nullptr;
    submitInfo.pWaitDstStageMask = nullptr;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &device->commandBuffer;
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.pSignalSemaphores = nullptr;
    
    VK_CALL(QueueSubmit(device->queue, 1, &submitInfo, VK_NULL_HANDLE),
            "error submitting raygen launch command(s) to queue");
    VK_CALL(QueueWaitIdle(device->queue),
            "error waiting for queue to finish raygen launch copy");



#if 1
    VkMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask
      = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask
      = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_SHADER_READ_BIT;
    cmdBufBI = {};
    cmdBufBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    VK_CALL(BeginCommandBuffer(device->commandBuffer, &cmdBufBI),
            "error beginning cmd buffer for raygen launch");
    vkCmdPipelineBarrier(device->commandBuffer,
                         /*srcStageMask*/VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         /*dstStageMask*/VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         0,
                         1, &barrier,
                         // 0, nullptr,
                         0, nullptr,
                         0,nullptr);//1, &barrier);
    // VkBufferMemoryBarrier barrier = {};
    VK_CALL(EndCommandBuffer(device->commandBuffer),
            "could not end launch end buffer");
    
    submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = NULL;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = nullptr;
    submitInfo.pWaitDstStageMask = nullptr;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &device->commandBuffer;
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.pSignalSemaphores = nullptr;


    VK_CALL(QueueSubmit(device->queue, 1, &submitInfo, VK_NULL_HANDLE),
            "error submitting raygen launch command(s) to queue");
    VK_CALL(QueueWaitIdle(device->queue),
            "error waiting for queue to finish raygen launch copy");
#endif
  }

  void Compute::buildPipeline()
  {
    VkPipelineShaderStageCreateInfo shaderStage = getShaderStage();
    
    // ==================================================================
    // layout
    // ==================================================================
    if (pipelineLayout != VK_NULL_HANDLE) {
      vkDestroyPipelineLayout(device->handle, pipelineLayout, nullptr);
      pipelineLayout = VK_NULL_HANDLE;
    }
    
    static VkPushConstantRange pushConstantRange = {};
    pushConstantRange.size//   = (int)context->pushConstantsSize;
      = std::max(sizeof(float),size);
    pushConstantRange.offset = 0;
    pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkPipelineLayoutCreateInfo layoutCI = {};
    layoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    
    static DescriptorPool      uniformBuffersPool(context->device,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    static DescriptorSetLayout uniformBuffersLayout(context->device);
    static DescriptorSet       uniformBuffersDS(context->device);
    
    std::vector<VkDescriptorSetLayoutBinding> bindings = {
      {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER/*_DYNAMIC*/, 1,
       VK_SHADER_STAGE_COMPUTE_BIT
       , nullptr}
    };
    uniformBuffersPool.create((int)bindings.size());
    uniformBuffersLayout.create(bindings);
    uniformBuffersDS.create(uniformBuffersPool,uniformBuffersLayout);
    
    layoutCI.pSetLayouts = &uniformBuffersLayout.handle;
    layoutCI.setLayoutCount = 1;//(uint32_t)dsLayouts.size();
    layoutCI.pushConstantRangeCount = 1;
    layoutCI.pPushConstantRanges = &pushConstantRange;
      
    VK_CALL(CreatePipelineLayout(device->handle,
                                 &layoutCI,
                                 nullptr, &pipelineLayout),
            "could not create pipeline");

    {
      VkComputePipelineCreateInfo ci = {};
      ci.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
      ci.layout = pipelineLayout;
      ci.flags = 0;
      ci.stage = shaderStage;
      VK_CHECK(device->vkCreateComputePipelines(device->handle,
                                                /*cache*/VK_NULL_HANDLE,
                                                1,
                                                &ci,
                                                nullptr,
                                                &pipeline),
             "could not create pipeline");
    }
    std::cout << "#vkn: compute pipeline built." << std::endl;
  }
    
} // ::vkn
