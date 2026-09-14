// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "vulkanite/RTPipeline.h"
#include "vulkanite/SBTObject.h"
#include "vulkanite/ShaderProgram.h"

namespace vkn {

  struct RayGenType : public SBTObjectType {
    typedef std::shared_ptr<RayGenType> SP;

    RayGenType(RTPipeline *pipeline,
               ShaderModule *shaderModule,
               const std::string &entryPoint,
               size_t dataSize);
    virtual ~RayGenType();
    
    std::string toString() override;
    
    ShaderProgram::SP program;
  };
  
  struct RayGen : public SBTObject {
    typedef std::shared_ptr<RayGen> SP;
    
    RayGen(RayGenType *type);
    virtual ~RayGen();
    
    std::string toString() override;
    
    RayGenType::SP type;
    int sbtIndex = -1;
  };
    
} // ::vkn


