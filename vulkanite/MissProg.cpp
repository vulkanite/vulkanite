// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#include "vulkanite/MissProg.h"
#include "vulkanite/Context.h"

namespace vkn {

  MissProgType::MissProgType(RTPipeline *pipeline,
                         ShaderModule *shaderModule,
                         const std::string &fctName,
                         size_t dataSize)
    : SBTObjectType(pipeline,dataSize),
      program(shaderModule->get(fctName,VK_SHADER_STAGE_MISS_BIT_KHR))
  {
    LOG_CONSTRUCTION(MissProgType,this);

    pipeline->track(this);
  }

  MissProgType::~MissProgType()
  {
    pipeline->forget(this);

    LOG_DESTRUCTION(MissProgType,this);
  }

  std::string MissProgType::toString() { return "MissProgType"; }
  std::string MissProg::toString() { return "MissProg"; }

  MissProg::MissProg(MissProgType *type)
    : SBTObject(type),
      type(type->asSP<MissProgType>())
  {
    LOG_CONSTRUCTION(MissProg,this);

    auto pipeline = &context->rtPipeline;
    this->sbtIndex = pipeline->missProgs.insert(this);
  }
  
  MissProg::~MissProg()
  {
    type.reset();
    auto pipeline = &context->rtPipeline;
    pipeline->missProgs.release(this->sbtIndex);

    LOG_DESTRUCTION(MissProg,this);
  }
  
} // ::vkn
