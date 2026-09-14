// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#include "prime/Context.h"
#include "prime/Group.h"
#include "prime/Mesh.h"
#include "prime/Model.h"
#include "prime/deviceCode.h"

extern std::vector<uint8_t> vkprime_embedded_spirv;

namespace vp {

  Context::Context(int32_t gpuID)
    : gpuID(gpuID)
  {
    VKNDriverInstance driver
      = vknInitialize(VKN_DEVICE_TYPE_DEFAULT);
    VKNDevice device
      = vknDeviceGet(driver,gpuID);
    this->vkn = vknContextCreate(device,sizeof(PushConstants));
    VKNShaderModule sm
      = vknShaderModuleCreate(vkn,
                              vkprime_embedded_spirv.data(),
                              vkprime_embedded_spirv.size());
    
    meshGT = vknGeomTypeCreate(vkn,VKN_GEOM_TRIANGLES,
                               sizeof(TrianglesGeomData));
    vknGeomTypeSetClosestHit(meshGT,sm,"TriangleMesh_CH");
    vknGeomTypeSetAnyHit(meshGT,sm,"TriangleMesh_AH");

    mp = vknMissProgCreate(vkn,sm,"missProg",sizeof(MissProgData));
    rg = vknRayGenCreate(vkn,sm,"traceRays",sizeof(RayGenData));
    vknBuildPrograms(vkn);
    vknBuildPipeline(vkn);
  }

  Model *Context::createModel(const std::vector<Group *> &groups,
                              const float *xfms)
  {
    return new Model(this,groups,xfms);
  }
  
  Group *Context::createGroup(const std::vector<Mesh *> &meshes)
  { return new Group(this,meshes); }
  
  Mesh *Context::createMesh(uint32_t geomID,
                            const float3 *vertices,
                            int numVertices,
                            const int3 *indices,
                            int numIndices)
  {
    return new Mesh(this,geomID,
                    vertices,numVertices,
                    indices,numIndices);
  }

}

