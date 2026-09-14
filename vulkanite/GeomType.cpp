// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#include "vulkanite/GeomType.h"
#include "vulkanite/Context.h"
#include "vulkanite/Triangles.h"
#include "vulkanite/UserGeom.h"

namespace vkn {

  GeomType::GeomType(RTPipeline *pipeline,
                     VKNGeomKind kind,
                     size_t dataSize)
    : SBTObjectType(pipeline,dataSize),
      kind(kind)
  {
    pipeline->track(this);
  }

  GeomType::~GeomType()
  {
    pipeline->forget(this);
  }
    
  void GeomType::setBoundsProg(ShaderModule *shaderModule,
                               const std::string &entryPoint)
  {
    boundsKernel = std::make_shared<Compute>
      (shaderModule,entryPoint,dataSize);
  }
  
  void GeomType::setIS(ShaderModule *shaderModule,
                       const std::string &entryPoint)
  {
    this->is = shaderModule->get
      (entryPoint,VK_SHADER_STAGE_INTERSECTION_BIT_KHR);
  }
  
  void GeomType::setAH(ShaderModule *shaderModule,
                       const std::string &entryPoint)
  {
    this->ah = shaderModule->get(entryPoint,VK_SHADER_STAGE_ANY_HIT_BIT_KHR);
  }
  
  void GeomType::setCH(ShaderModule *shaderModule,
                       const std::string &entryPoint)
  {
    this->ch = shaderModule->get(entryPoint,VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
  }

  std::shared_ptr<Geom> GeomType::createInstance()
  {
    switch (kind) {
    case VKN_GEOM_TRIANGLES:
      return std::make_shared<TrianglesGeom>(this);
    case VKN_GEOM_USER:
      return std::make_shared<UserGeom>(this);
    default:
      throw std::runtime_error("unsupported geometry kind!?");
    }
  }

  Geom::Geom(GeomType *type)
    : SBTObject(type),
      type(type->asSP<GeomType>())
  {}
  
} // ::vkn
