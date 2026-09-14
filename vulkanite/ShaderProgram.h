// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "vulkanite/ShaderModule.h"

namespace vkn {
  
  struct ShaderProgram : public Object {
    typedef std::shared_ptr<ShaderProgram> SP;
    
    ShaderProgram(ShaderModule *shaderModule,
                  const std::string &entryPoint,
                  VkShaderStageFlagBits stage);
                  // VkShaderStageFlagBits type);
    //VkStructureType type);
    virtual ~ShaderProgram();

    std::string toString() override;
    VkPipelineShaderStageCreateInfo getShaderStage();
    
    std::string             const entryPoint;
    ShaderModule::SP        const shaderModule;
    // VkStructureType type;
    VkShaderStageFlagBits stage;

    // computed/set by/in pipline::build
    int shaderStageIndex = -1;
  };

}
