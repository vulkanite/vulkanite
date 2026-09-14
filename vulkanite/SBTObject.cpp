// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#include "vulkanite/ShaderModule.h"
#include "vulkanite/RTPipeline.h"
#include "vulkanite/SBTObject.h"

namespace vkn {
  
  SBTObjectType::SBTObjectType(RTPipeline *pipeline,
                               size_t dataSize)
    : Object(pipeline->context),
        pipeline(pipeline),
      dataSize(dataSize)
  {}
  
  SBTObjectType::~SBTObjectType() {}
    
  std::string SBTObjectType::toString()
  { return "SBTObjectType"; }
  
  SBTObject::SBTObject(SBTObjectType *type)
    : Object(type->context),
      type(type->asSP<SBTObjectType>()),
      data(type->dataSize)
  {}
  
  SBTObject::~SBTObject()
  {}
  
  std::string SBTObject::toString()
  { return "SBTObject"; }
    
  void SBTObject::setData(const void *ptr)
  { memcpy(data.data(),ptr,data.size()); }
  
}
