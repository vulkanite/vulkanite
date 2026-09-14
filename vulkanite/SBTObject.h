// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "vulkanite/ShaderModule.h"

namespace vkn {
  struct Context;
  struct RTPipeline;
  
  struct SBTObjectType : public Object {
    typedef std::shared_ptr<SBTObjectType> SP;

    SBTObjectType(RTPipeline *pipeline,
                  size_t dataSize);
    virtual ~SBTObjectType();
    std::string toString() override;
    
    size_t                  const dataSize;
    RTPipeline             *pipeline;
    int shaderGroupIndex = -1;
  };
  
  struct SBTObject : public Object {
    typedef std::shared_ptr<SBTObject> SP;
    
    SBTObject(SBTObjectType *type);
    virtual ~SBTObject();
    
    std::string toString() override;
    
    void setData(const void *ptr);
    void writeData(uint8_t *where)
    { memcpy(where,data.data(),data.size()); }
    
    SBTObjectType::SP       type;
    std::vector<uint8_t>    data;
  };

}
