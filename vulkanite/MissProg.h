// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "vulkanite/RTPipeline.h"
#include "vulkanite/SBTObject.h"
#include "vulkanite/ShaderProgram.h"

namespace vkn {

  struct MissProgType : public SBTObjectType {
    typedef std::shared_ptr<MissProgType> SP;

    MissProgType(RTPipeline *pipeline,
               ShaderModule *shaderModule,
               const std::string &entryPoint,
               size_t dataSize);
    virtual ~MissProgType();
    
    std::string toString() override;
    
    ShaderProgram::SP program;
  };
  
  struct MissProg : public SBTObject {
    typedef std::shared_ptr<MissProg> SP;
    
    MissProg(MissProgType *type);
    virtual ~MissProg();
    
    std::string toString() override;
    
    MissProgType::SP type;
    int sbtIndex = -1;
  };
    
} // ::vkn


