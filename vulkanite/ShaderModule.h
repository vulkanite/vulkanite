// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "vulkanite/Object.h"
#include "vulkanite/driver/spirv_reflect.h"

#define DO_REFLECT 0

namespace vkn {
  
  struct Context;
  struct ShaderProgram;
  struct ShaderModule;
  struct RTPipeline;

  struct ShaderProgram;
  
  struct ShaderModule : public Object {
    typedef std::shared_ptr<ShaderModule> SP;
    
    ShaderModule(RTPipeline *pipeline, 
                 /*! embedded, precompiled binary spir-v code */
                 const std::vector<uint8_t> &code);
    virtual ~ShaderModule();

    std::string toString() override { return "ShaderModule"; }
    std::shared_ptr<ShaderProgram> get(const std::string &entryPoint,
                                       VkShaderStageFlagBits stage);
    std::map<std::string,std::weak_ptr<ShaderProgram>> programs;

    VkShaderModule            handle = 0;
    RTPipeline               *pipeline = 0;
                 
    // from spirv introspection - not sure we actually need this?
#if DO_REFLECT
    std::set<std::string>  entryPoints;
#endif
  };

} // ::vkn
    
