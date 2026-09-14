// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#include "vulkanite/RayGen.h"
#include "vulkanite/Context.h"

namespace vkn {

  RayGenType::RayGenType(RTPipeline *pipeline,
                         ShaderModule *shaderModule,
                         const std::string &fctName,
                         size_t dataSize)
    : SBTObjectType(pipeline,dataSize),
      program(shaderModule->get(fctName,VK_SHADER_STAGE_RAYGEN_BIT_KHR))
  {
    LOG_CONSTRUCTION(RayGenType,this);

    pipeline->track(this);
  }

  RayGenType::~RayGenType()
  {
    pipeline->forget(this);

    LOG_DESTRUCTION(RayGenType,this);
  }
 
  std::string RayGenType::toString() { return "RayGenType"; }
  std::string RayGen::toString() { return "RayGen"; }

  RayGen::RayGen(RayGenType *type)
    : SBTObject(type),
      type(type->asSP<RayGenType>())
  {
    LOG_CONSTRUCTION(RayGen,this);

    auto pipeline = &context->rtPipeline;
    this->sbtIndex = pipeline->rayGens.insert(this);
  }
  
  RayGen::~RayGen()
  {
    type.reset();
    auto pipeline = &context->rtPipeline;
    pipeline->rayGens.release(this->sbtIndex);

    LOG_DESTRUCTION(RayGen,this);
  }
  
} // ::vkn
