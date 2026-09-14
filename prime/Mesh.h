// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#include "prime/vkn-prime-common.h"

namespace vp {

  struct Context;
  
  struct Mesh {
    Mesh(Context *context,
         int geomID,
         const float3 *vertices,
         int numVertices,
         const int3 *indices,
         int numIndices);

    Context *const context;
    int      const geomID;
    VKNGeom   vkn = 0;
    VKNBuffer verticesBuffer = 0;
    VKNBuffer indicesBuffer = 0;
  };
  
}
