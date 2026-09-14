// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#include "prime/Context.h"
#include "prime/Model.h"
#include "deviceCode.h"

using namespace vp;

#define WARN_MISSING() std::cout << "#vp: not-yet-implemented function " << __PRETTY_FUNCTION__ << std::endl;

VP_API
VPContext vpContextCreate(/*! which GPU to use, '0' being the first
                           *  GPU, '1' the second, etc. '-1' means
                           *  'use host CPU only' */
                          int32_t gpuToUse)
{
  return (VPContext)new Context(gpuToUse);
}

VP_API
void  vpContextDestroy(VPContext context)
{
  WARN_MISSING();
}

VP_API
VPGroup vpGroupCreate(VPContext _context,
                      VPGeom *geoms,
                      int numGeoms)
{
  Context *context = (Context *)_context;
  std::vector<Mesh *> meshes;
  for (int i=0;i<numGeoms;i++) {
    auto geom = geoms[i];
    assert(geom);
    meshes.push_back((Mesh *)geom);
  }
  return (VPGroup)context->createGroup(meshes);
}


VP_API VPGeom vpMeshCreate(VPContext _context,
                           /*! user provided data value to be stored
                             in the hit */
                           uint64_t userGeomID,
                           /* vertex array */
                           const float *vertices,
                           size_t numVertices,
                           size_t sizeOfVertexInBytes,
                           /* index array */
                           const int   *indices,
                           size_t numTriangles,
                           size_t sizeOfIndexStructInBytes)
{
  Context *context = (Context *)_context;
  assert(sizeOfVertexInBytes == sizeof(float3));
  assert(sizeOfIndexStructInBytes == sizeof(int3));
  return (VPGeom)context->createMesh
    (userGeomID,
     (const float3*)vertices,
     numVertices,
     (const int3*)indices,
     numTriangles);
}

VP_API
VPModel vpModelCreate(VPContext    _context,
                      VPGroup     *_groups,
                      VPTransform *xfms,
                      int          numInstances)
{
  Context *context = (Context *)_context;
  std::vector<Group *> groups;
  for (int i=0;i<numInstances;i++) {
    Group *group = (Group *)_groups[i];
    assert(group);
    groups.push_back(group);
  }
  return (VPModel)context->createModel
    (groups,(float*)xfms);
}

VP_API
void vpTrace(/*! the model to trace into */
             VPModel _model,
             /*! number of rays to trace - both arrays must have as
              *  many entries */
             size_t numRays,
             /*! array of rays; must be (at least) as many as
              *  'numRays' */
             VPRay *arrayOfRays,
             /*! array of where to write the results; as many as
              *  'numRays' */
             VPHit *arrayOfHits,
             /*! trace flags that can fine-tune how the trace
              *  executes. */
             VPTraceMode traceMode)
{
  Model *model = (Model *)_model;
  Context *context = model->context;
  PushConstants pc;
  #if 0
  pc.rays = (PrimeRay*)arrayOfRays;
  pc.rayIDs = 0;
  pc.hits = (PrimeHit *)arrayOfHits;
  pc.hitIDs = 0;
  pc.traceMode = (int)traceMode;
  pc.world = vknAccelGetTraversableHandle(model->vkn);
  pc.numRays = numRays;
  #endif

  RayGenData rg;
  rg.rays = (PrimeRay*)arrayOfRays;
  rg.rayIDs = 0;
  rg.hits = (PrimeHit *)arrayOfHits;
  rg.hitIDs = 0;
  rg.traceMode = (int)traceMode;
  rg.world = vknAccelGetTraversableHandle(model->vkn);
  rg.numRays = numRays;
  vknRayGenSetData(context->rg,&rg);
  vknBuildSBT(context->vkn);
  
  vknRayGenLaunch2D(context->rg,
                    context->mp,
                    numRays,1,
                    &pc);
}


VP_API void *vpMalloc(VPContext _context,
                      size_t size)
{
  Context *context = (Context *)_context;
  VKNBuffer buffer = vknDeviceBufferCreate(context->vkn,size,0);
  return vknBufferGetPointer(buffer);
}

