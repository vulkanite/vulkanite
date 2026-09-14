// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#include "prime/Context.h"
#include "prime/Mesh.h"
#include "deviceCode.h"

namespace vp {
  Mesh::Mesh(Context *context,
             int geomID,
             const float3 *vertices,
             int numVertices,
             const int3 *indices,
             int numIndices)
    : context(context),
      geomID(geomID)
  {
    verticesBuffer
      = vknDeviceBufferCreate(context->vkn,
                              numVertices*sizeof(float3),
                              vertices);
    indicesBuffer
      = vknDeviceBufferCreate(context->vkn,
                              numIndices*sizeof(float3),
                              indices);
    this->vkn
      = vknGeomCreate(context->meshGT);
    vknTrianglesSetVertices(vkn,
                            numVertices,
                            vknBufferGetPointer(verticesBuffer),
                            sizeof(float3));
    vknTrianglesSetIndices(vkn,
                            numIndices,
                            vknBufferGetPointer(indicesBuffer),
                            sizeof(int3));
    TrianglesGeomData data;
    data.geomID = geomID;
    vknGeomSetData(vkn,&data);
  }
  
}

