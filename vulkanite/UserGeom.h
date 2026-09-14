// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "vulkanite/GeomType.h"
#include "vulkanite/Accel.h"

namespace vkn {
    
  struct UserGeom : public Geom {
    typedef std::shared_ptr<UserGeom> SP;
    
    UserGeom(GeomType *gt);
    virtual ~UserGeom();
    
    std::string toString() override { return "UserGeom"; }
    int primCount = 0;
  };

  struct UserGeomAccel : public GeomAccel {
    UserGeomAccel(Context *context,
                       const std::vector<UserGeom::SP> &geoms);
    virtual ~UserGeomAccel();
    
    std::string toString() override;

    void build() override;

    const std::vector<Geom::SP> &getGeoms() const override
    { return (const std::vector<Geom::SP> &)geoms; }
    
    const std::vector<UserGeom::SP> geoms;
  };
  
} // ::vkn


