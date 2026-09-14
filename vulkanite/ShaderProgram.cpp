// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#include "vulkanite/RayGen.h"
#include "vulkanite/Context.h"
#include "vulkanite/RTPipeline.h"
#include "vulkanite/ShaderProgram.h"

namespace vkn {

  ShaderProgram::ShaderProgram(ShaderModule *shaderModule,
                               const std::string &entryPoint,
                               VkShaderStageFlagBits stage)
    : Object(shaderModule->context),
      entryPoint(entryPoint),
      shaderModule(shaderModule->asSP<ShaderModule>()),
      stage(stage)
  {
    LOG_CONSTRUCTION(ShaderProgram,this);

#if DO_REFLECT
    if (shaderModule->entryPoints.find(entryPoint)
        == shaderModule->entryPoints.end())
      throw std::runtime_error
        ("given shader module does not have an entry point of name '"+entryPoint+"'");
#endif
    shaderModule->pipeline->track(this);
  }
  
  ShaderProgram::~ShaderProgram()
  {
    shaderModule->pipeline->forget(this);
    
    LOG_DESTRUCTION(ShaderProgram,this);
  }

  std::string ShaderProgram::toString()
  {
    return "ShaderProgram";
  }

  VkPipelineShaderStageCreateInfo ShaderProgram::getShaderStage()
  {
    VkPipelineShaderStageCreateInfo shaderStage{};
    shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStage.stage = stage;
    shaderStage.module = shaderModule->handle;
    shaderStage.pName = entryPoint.c_str();
    
    assert(shaderStage.module);
    assert(shaderStage.pName);
    
    return shaderStage;
  }
  
}

