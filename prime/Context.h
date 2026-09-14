// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "prime/vkn-prime-common.h"

namespace vp {
  struct Group;
  struct Mesh;
  struct Model;
  
  struct Context {
    Context(int32_t gpuID);

    Group *createGroup(const std::vector<Mesh *> &meshes);
    Mesh *createMesh(uint32_t geomID,
                     const float3 *vertices,
                     int numVertices,
                     const int3 *indices,
                     int numIndices);
    Model *createModel(const std::vector<Group *> &groups,
                       const float *xfms);
    
    /*! which GPU to use, '0' being the first
     *  GPU, '1' the second, etc. '-1' means
     *  'use host CPU only' */
    int32_t const gpuID;
    VKNContext vkn = 0;
    VKNGeomType meshGT = 0;
    VKNMissProg mp = 0;
    VKNRayGen   rg = 0;
  };
  
}
