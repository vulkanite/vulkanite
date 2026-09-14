// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#include "vulkanite/RTPipeline.h"
#include "vulkanite/RayGen.h"
#include "vulkanite/MissProg.h"
#include "vulkanite/GeomType.h"
#include "vulkanite/Context.h"
#include "vulkanite/Image.h"
#include "vulkanite/Sampler.h"

namespace vkn {

  template<typename T>
  struct DescStuff;

  template<>
  struct DescStuff<VkSampler> {
    static VkDescriptorType descriptorType()
    { return VK_DESCRIPTOR_TYPE_SAMPLER; }
    static std::string typeName() { return "Sampler"; };
    typedef VkDescriptorImageInfo VkDescriptorInfoT;
  };
  template<>
  struct DescStuff<VkImage> {
    static VkDescriptorType descriptorType()
    { return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ; }
    static std::string typeName() { return "Image"; };
    typedef VkDescriptorImageInfo VkDescriptorInfoT;
  };

  template<typename VkHandle, typename Type>
  DSHelper<VkHandle,Type>::DSHelper(Context *context)
    : context(context),
      pool(context->device,DescStuff<VkHandle>::descriptorType()),
      layout(context->device),
      descriptorSet(context->device)
  {}
    
  template<typename VkHandle, typename Type>
  void DSHelper<VkHandle,Type>::update()
  {}

  template<typename VkHandle, typename Type>
  int  DSHelper<VkHandle,Type>::insert(Type *t)
  {
    dirty = true;
    return objects.track(t);
  }
  
  template<typename VkHandle, typename Type>
  void DSHelper<VkHandle,Type>::createLayout()
  {
    std::vector<VkDescriptorSetLayoutBinding> bindings = {
      {0, DescStuff<VkHandle>::descriptorType(),
       (uint32_t)std::max((size_t)1,objects.data.size()),
       VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR
       | VK_SHADER_STAGE_ANY_HIT_BIT_KHR
       | VK_SHADER_STAGE_INTERSECTION_BIT_KHR
       | VK_SHADER_STAGE_MISS_BIT_KHR
       | VK_SHADER_STAGE_COMPUTE_BIT
       | VK_SHADER_STAGE_RAYGEN_BIT_KHR,
       nullptr}
    };
    layout.create(bindings);
  }

  template<typename VkHandle, typename Type>
  std::string DSHelper<VkHandle,Type>::toString()
  { return "DSHelper<"+DescStuff<VkHandle>::typeName()+">"; };
  
  template<typename VkHandle, typename Type>
  void DSHelper<VkHandle,Type>::createDS()
  {
    if (dsSize != objects.data.size()) {
      destroy();
      pool.create(objects.data.size());
      descriptorSet.create(pool,layout);

      using InfoT = VkDescriptorImageInfo;
      std::vector<InfoT> descriptorInfos(objects.data.size());
      for (int i=0;i<objects.data.size();i++)
        if (objects.data[i]) {
          descriptorInfos[i] = objects.data[i]->getDescriptorInfo();
        } else {
          descriptorInfos[i] = {};
        }
      
      VkWriteDescriptorSet writeDescriptorSet;
      writeDescriptorSet = {};
      writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      writeDescriptorSet.dstBinding = 0;
      writeDescriptorSet.dstArrayElement = 0;
      writeDescriptorSet.descriptorType
        = DescStuff<VkHandle>::descriptorType();
      writeDescriptorSet.descriptorCount = (int)objects.data.size();
      // one UBO per descriptor here
      // if (DSHelper<VkHandle>::is_image) {
      writeDescriptorSet.pImageInfo = descriptorInfos.data();
      // } else {
      // }
      // writeDescriptorSet.pBufferInfo = bufferInfos.data();//&uniformBufferDescriptor;
      // writeDescriptorSet.pImageInfo = 0;
      writeDescriptorSet.dstSet = descriptorSet.handle;//computeRecordDescriptorSet;
      
      // We'll write these descriptors now, but the actual
      // recordBuffer will be written to later.
      auto device = context->device;
      vkUpdateDescriptorSets(device->handle, 1,
                             &writeDescriptorSet, 0, nullptr);

      
    } else
      std::cout << "<EMPTY> descriptor set!?" << std::endl;
  }
  
  template<typename VkHandle, typename Type>
  void DSHelper<VkHandle,Type>::remove(int ID)
  {
    objects.forget(ID);
  }
    
  template struct DSHelper<VkSampler,Sampler>;
  template struct DSHelper<VkImage,Image>;

  
  RTPipeline::RTPipeline(Context *context)
    : context(context),
      device(context->device),
      samplers(context),
      image2Ds(context)
  {
    properties = {};
    properties.sType
      = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
    properties.pNext = nullptr;//&subgroupProperties;
    
    // get properties
    VkPhysicalDeviceProperties2 devProps{};
    devProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    devProps.pNext = &this->properties;
    vkGetPhysicalDeviceProperties2
      (device->physicalDevice->handle, &devProps);
  }

  RTPipeline::~RTPipeline()
  {
    std::cout << "#vkn: RTPipeline is dying" << std::endl;
    if (pipeline) {
      vkDestroyPipeline(device->getHandle(),pipeline,nullptr);
      pipeline = 0;
    }
    if (layout) {
      vkDestroyPipelineLayout(device->getHandle(),layout,nullptr);
      layout = 0;
    }
    if (rayGenDescriptorSetLayout) {
      vkDestroyDescriptorSetLayout(device->getHandle(),rayGenDescriptorSetLayout,nullptr);
      rayGenDescriptorSetLayout = 0;
    }
  }

  void RTPipeline::track(ShaderProgram *program)
  {
    programs.insert(program);
  }
  
  void RTPipeline::forget(ShaderProgram *program)
  {
    programs.erase(program);
    // programs.erase(programs.find(program));
  }
  
  void RTPipeline::track(GeomAccel *accel)
  {
    geomAccels.insert(accel);
  }
  
  void RTPipeline::forget(GeomAccel *accel)
  {
    geomAccels.erase(accel);
  }
  
  void RTPipeline::track(RayGenType *rgt)
  {
    rayGenTypes.insert(rgt);
  }
  
  void RTPipeline::forget(RayGenType *rgt)
  {
    rayGenTypes.erase(rgt);
    // rayGenTypes.erase(rayGenTypes.find(rgt));
  }
  
  void RTPipeline::track(GeomType *gt)
  {
    geomTypes.insert(gt);
  }
  
  void RTPipeline::forget(GeomType *gt)
  {
    geomTypes.erase(geomTypes.find(gt));
  }

  void RTPipeline::track(MissProgType *rgt)
  {
    missProgTypes.insert(rgt);
  }
  
  void RTPipeline::forget(MissProgType *rgt)
  {
    missProgTypes.erase(rgt);
    // rayGenTypes.erase(rayGenTypes.find(rgt));
  }
  
  
  
  void RTPipeline::gatherPrograms(std::vector<VkPipelineShaderStageCreateInfo> &shaderStages,
                                std::vector<VkRayTracingShaderGroupCreateInfoKHR> &shaderGroups)
  {
    VkRayTracingShaderGroupCreateInfoKHR emptySG = {};
    emptySG.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    emptySG.type  = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    emptySG.generalShader      = VK_SHADER_UNUSED_KHR;
    emptySG.closestHitShader   = VK_SHADER_UNUSED_KHR;
    emptySG.anyHitShader       = VK_SHADER_UNUSED_KHR;
    emptySG.intersectionShader = VK_SHADER_UNUSED_KHR;
      
    shaderStages.clear();
    for (auto prog : programs) {
      // do NOT include compute programs
      if (prog->stage & VK_SHADER_STAGE_COMPUTE_BIT)
        continue;
      prog->shaderStageIndex = shaderStages.size();
      shaderStages.push_back(prog->getShaderStage());
    }
    
    shaderGroups.clear();

    for (auto rg : missProgTypes) {
      auto sg = emptySG;
      sg.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
      sg.generalShader = rg->program->shaderStageIndex;
      
      rg->shaderGroupIndex = shaderGroups.size();
      shaderGroups.push_back(sg);
    }

    for (auto rg : rayGenTypes) {
      auto sg = emptySG;
      sg.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
      sg.generalShader = rg->program->shaderStageIndex;
      
      rg->shaderGroupIndex = shaderGroups.size();
      shaderGroups.push_back(sg);
    }

    for (auto gt : geomTypes) {
      auto sg = emptySG;
      sg.generalShader    = VK_SHADER_UNUSED_KHR;
      switch(gt->kind) {
      case VKN_GEOM_TRIANGLES:
        sg.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
        break;
      case VKN_GEOM_USER:
        sg.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR;
        break;
      default:
        throw std::runtime_error("not implemented");
      }
      sg.closestHitShader
        = gt->ch
        ? gt->ch->shaderStageIndex
        : VK_SHADER_UNUSED_KHR;
      
      sg.anyHitShader
        = gt->ah
        ? gt->ah->shaderStageIndex
        : VK_SHADER_UNUSED_KHR;
      
      if (gt->kind == VKN_GEOM_USER) {
        if (!gt->is)
          throw std::runtime_error
            ("trying to create user geometry type without an intersection shader...");
        sg.intersectionShader
          = gt->is->shaderStageIndex;
      }
      
      gt->shaderGroupIndex = shaderGroups.size();
      shaderGroups.push_back(sg);
    }
  }

  void RTPipeline::writeHeader(uint8_t *where, int shaderGroupIndex)
  {
    const size_t sbtHeaderSize = properties.shaderGroupHandleSize;
    memcpy(where,
           (uint8_t*)shaderHandleStorage.data()+shaderGroupIndex*sbtHeaderSize,
           sbtHeaderSize);
  }
  
  void RTPipeline::build()
  {
    if (pipeline) {
      vkDestroyPipeline(device->getHandle(),pipeline,nullptr);
      pipeline = 0;
    }
    if (layout) {
      vkDestroyPipelineLayout(device->getHandle(),layout,nullptr);
      layout = 0;
    }
    if (rayGenDescriptorSetLayout) {
      vkDestroyDescriptorSetLayout(device->getHandle(),
                                   rayGenDescriptorSetLayout,nullptr);
      rayGenDescriptorSetLayout = 0;
    }
        
    VkRayTracingPipelineInterfaceCreateInfoKHR interfaceCI = {};
    interfaceCI.sType
      = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_INTERFACE_CREATE_INFO_KHR;
    interfaceCI.maxPipelineRayPayloadSize
      = /*??*/2;
    interfaceCI.maxPipelineRayHitAttributeSize
      = /*??*/2;

    // ==================================================================
    // layout
    // ==================================================================
    if (layout != VK_NULL_HANDLE) {
      vkDestroyPipelineLayout(device->handle, layout, nullptr);
      layout = VK_NULL_HANDLE;
    }
      
    VkDescriptorSetLayoutBinding binding{};
    // binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    binding.descriptorCount = 1;   // we only have one of these bound at any point in time
    binding.binding = 0;
    binding.stageFlags
      = VK_SHADER_STAGE_RAYGEN_BIT_KHR
      | VK_SHADER_STAGE_COMPUTE_BIT;
      
    std::vector<VkDescriptorSetLayoutBinding> dsLayoutBindings = {binding};
      
    VkDescriptorSetLayoutCreateInfo dsLayoutCI{};
    dsLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    dsLayoutCI.pBindings = dsLayoutBindings.data();
    dsLayoutCI.bindingCount = (uint32_t)dsLayoutBindings.size();
    dsLayoutCI.pNext = nullptr;
    VK_CALL(CreateDescriptorSetLayout(device->handle,
                                      &dsLayoutCI, nullptr,
                                      &rayGenDescriptorSetLayout),
            "could not create ray gen descriptor set layout");
    std::cout << "#vkn: ray gen descriptor set layout created" << std::endl;
    assert(rayGenDescriptorSetLayout);

    static VkPushConstantRange pushConstantRange = {};
    pushConstantRange.size   = maxPushConstantSize;

    pushConstantRange.offset = 0;
    pushConstantRange.stageFlags
      = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR
      | VK_SHADER_STAGE_ANY_HIT_BIT_KHR
      | VK_SHADER_STAGE_INTERSECTION_BIT_KHR
      | VK_SHADER_STAGE_MISS_BIT_KHR
      | VK_SHADER_STAGE_CALLABLE_BIT_KHR
      | VK_SHADER_STAGE_RAYGEN_BIT_KHR
      | VK_SHADER_STAGE_COMPUTE_BIT
      ;

    // PRINT(samplers.layout.handle);
    samplers.createLayout();
    samplers.createDS();
    image2Ds.createLayout();
    image2Ds.createDS();

    std::vector<VkDescriptorSetLayout> dsLayouts =
      {
       // /* not actually using this one, but VK wants something valid...*/,
        rayGenDescriptorSetLayout,
        samplers.layout.handle,
        image2Ds.layout.handle
       // samplerDescriptorSetLayout,
       // texture1DDescriptorSetLayout,
       // texture2DDescriptorSetLayout,
       // texture3DDescriptorSetLayout,
       // bufferDescriptorSetLayout
      };
    VkPipelineLayoutCreateInfo layoutCI = {};
    layoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutCI.setLayoutCount = (uint32_t)dsLayouts.size();
    layoutCI.pSetLayouts = dsLayouts.data();
    layoutCI.pushConstantRangeCount = 1;
    layoutCI.pPushConstantRanges = &pushConstantRange;
      
    VK_CALL(CreatePipelineLayout(device->handle,
                                 &layoutCI,
                                 nullptr, &layout),
            "could not create pipeline");
    assert(layout);
      
    // ==================================================================
    // ==================================================================
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups;
    gatherPrograms(shaderStages,shaderGroups);
    VkRayTracingPipelineCreateInfoKHR pipelineCI = {};
    pipelineCI.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
    pipelineCI.stageCount = (uint32_t)shaderStages.size();
    pipelineCI.pStages = shaderStages.data();
    pipelineCI.groupCount = (uint32_t)shaderGroups.size();
    pipelineCI.pGroups = shaderGroups.data();
    pipelineCI.maxPipelineRayRecursionDepth = /*??*/1;
    pipelineCI.layout = layout;
    pipelineCI.pLibraryInterface = &interfaceCI;

    VK_CHECK(device->vkCreateRayTracingPipelines(device->handle,
                                                 VK_NULL_HANDLE,
                                                 VK_NULL_HANDLE,
                                                 1,
                                                 &pipelineCI,
                                                 nullptr,
                                                 &pipeline),
             "could not create pipeline");
    assert(pipeline);
      
    size_t shaderHandleSize
      = shaderGroups.size()*properties.shaderGroupHandleSize;
    
    shaderHandleStorage.resize(shaderHandleSize);

    VK_CHECK(device->vkGetRayTracingShaderGroupHandles
             (device->handle,
              pipeline, 0, (int)shaderGroups.size(),
              shaderHandleStorage.size(),
              shaderHandleStorage.data()),
             "could not get shader handles");

    std::cout << std::endl;
    std::cout << "#vkn: programs compiled and pipeline built." << std::endl;
  }

  
}
