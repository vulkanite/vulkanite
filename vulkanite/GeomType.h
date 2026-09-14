// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "vulkanite/SBTObject.h"
#include "vulkanite/ShaderProgram.h"
#include "vulkanite/Compute.h"

namespace vkn {

  struct Geom;
  struct GeomType;
  
  struct Geom : public SBTObject {
    Geom(GeomType *type);
    virtual ~Geom() = default;

    std::shared_ptr<GeomType> type;
  };
  
  struct GeomType : public SBTObjectType {
    typedef std::shared_ptr<GeomType> SP;
    
    GeomType(RTPipeline *pipeline,
             VKNGeomKind kind,
             size_t dataSize);
    virtual ~GeomType();
    
    std::string toString() override { return "GeomType"; }

    void setAH(ShaderModule *shaderModule,
               const std::string &entryPoint);
    void setCH(ShaderModule *shaderModule,
               const std::string &entryPoint);
    void setIS(ShaderModule *shaderModule,
               const std::string &entryPoint);
    void setBoundsProg(ShaderModule *shaderModule,
                       const std::string &entryPoint);

    std::shared_ptr<Geom> createInstance();
    
    ShaderProgram::SP ah, ch, is;
    Compute::SP boundsKernel;
    const VKNGeomKind kind;
  };
  
} // ::vkn


