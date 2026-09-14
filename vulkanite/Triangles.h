// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "vulkanite/GeomType.h"
#include "vulkanite/Accel.h"

namespace vkn {
    
  struct TrianglesGeom : public Geom {
    typedef std::shared_ptr<TrianglesGeom> SP;
    
    TrianglesGeom(GeomType *gt);
    virtual ~TrianglesGeom();
    
    std::string toString() override { return "TrianglesGeom"; }

    struct {
      union {
        void    *ptr = 0;
        VkDeviceAddress d_address;
      };
      size_t   stride    = 0;
      uint32_t count     = 0;
    } vertices, indices;
  };

  struct TrianglesGeomAccel : public GeomAccel {
    TrianglesGeomAccel(Context *context,
                       const std::vector<TrianglesGeom::SP> &geoms);
    virtual ~TrianglesGeomAccel();
    
    std::string toString() override;

    void build() override;

    const std::vector<Geom::SP> &getGeoms() const override
    { return (const std::vector<Geom::SP> &)geoms; }
    
    const std::vector<TrianglesGeom::SP> geoms;
  };
  
} // ::vkn


