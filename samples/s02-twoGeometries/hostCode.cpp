// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#include "vulkanite/vulkanite.h"
// our device-side data structures
#include "deviceCode.h"
#include <vector>
#include <iostream>
// external helper stuff for image output
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

#define LOG(message)                                            \
  std::cout << VKN_TERMINAL_GREEN;                               \
  std::cout << "#vkn.sample(main): " << message << std::endl;   \
  std::cout << VKN_TERMINAL_DEFAULT;
#define LOG_OK(message)                                         \
  std::cout << VKN_TERMINAL_LIGHT_GREEN;                         \
  std::cout << "#vkn.sample(main): " << message << std::endl;   \
  std::cout << VKN_TERMINAL_DEFAULT;

extern std::vector<uint8_t> s02_embedded_spirv;

const int NUM_VERTICES = 8;
float3 vertices_l[NUM_VERTICES] =
  {
    { -1.f,-1.f,-1.f },
    { -.2f,-1.f,-1.f },
    { -1.f,+1.f,-1.f },
    { -.2f,+1.f,-1.f },
    { -1.f,-1.f,+1.f },
    { -.2f,-1.f,+1.f },
    { -1.f,+1.f,+1.f },
    { +-.2f,+1.f,+1.f }
  };
float3 vertices_r[NUM_VERTICES] =
  {
    { +.2f,-1.f,-1.f },
    { +1.f,-1.f,-1.f },
    { +.2f,+1.f,-1.f },
    { +1.f,+1.f,-1.f },
    { +.2f,-1.f,+1.f },
    { +1.f,-1.f,+1.f },
    { +.2f,+1.f,+1.f },
    { +1.f,+1.f,+1.f }
  };

const int NUM_INDICES = 12;
int3 indices[NUM_INDICES] =
  {
    { 0,1,3 }, { 2,3,0 },
    { 5,7,6 }, { 5,6,4 },
    { 0,4,5 }, { 0,5,1 },
    { 2,3,7 }, { 2,7,6 },
    { 1,5,7 }, { 1,7,3 },
    { 4,0,2 }, { 4,2,6 }
  };

const char *outFileName = "s02-twoGeometries.png";
const int2 fbSize(800,600);
const float3 lookFrom(-2.f,-3.f,-4.f);
const float3 lookAt(0.f,0.f,0.f);
const float3 lookUp(0.f,1.f,0.f);
const float cosFovy = 0.66f;

int main(int ac, char **av)
{
  float3 dummy;
  LOG("vkn::ng example '" << av[0] << "' starting up");

  // create a context on the first device:
  VKNDriverInstance vkn = vknInitialize(VKN_DEVICE_TYPE_DEFAULT);
  VKNContext context = vknContextCreate(vknDeviceGet(vkn,0),sizeof(dummy));
  VKNShaderModule module = vknShaderModuleCreate(context,
                                                 s02_embedded_spirv.data(),
                                                 s02_embedded_spirv.size());

  // ##################################################################
  // set up all the *GEOMETRY* graph we want to render
  // ##################################################################

  // -------------------------------------------------------
  // declare geometry type
  // -------------------------------------------------------
  VKNGeomType trianglesGeomType
    = vknGeomTypeCreate(context,
                        VKN_GEOM_TRIANGLES,
                        sizeof(TrianglesGeomData));
  vknGeomTypeSetClosestHit(trianglesGeomType,
                           module,"TriangleMesh_CH");
  vknGeomTypeSetAnyHit(trianglesGeomType,
                           module,"TriangleMesh_AH");

  // ##################################################################
  // set up all the *GEOMS* we want to run that code on
  // ##################################################################

  LOG("building geometries ...");

  // ------------------------------------------------------------------
  // triangle mesh(es) - in this sample, two of them.
  // ------------------------------------------------------------------
  VKNBuffer vertexBuffer_l
    = vknDeviceBufferCreate(context,sizeof(float3)*NUM_VERTICES,vertices_l); 
  VKNBuffer vertexBuffer_r
    = vknDeviceBufferCreate(context,sizeof(float3)*NUM_VERTICES,vertices_r);
  VKNBuffer indexBuffer
    = vknDeviceBufferCreate(context,sizeof(int3)*NUM_INDICES,indices);
  
  // create and parametrize left geometry:
  VKNGeom trianglesGeom_l
    = vknGeomCreate(trianglesGeomType);
  vknTrianglesSetVertices(trianglesGeom_l,
                          NUM_VERTICES,
                          vknBufferGetPointer(vertexBuffer_l),
                          sizeof(float3));
  vknTrianglesSetIndices(trianglesGeom_l,
                         NUM_INDICES,
                         vknBufferGetPointer(indexBuffer),
                         sizeof(int3));
  TrianglesGeomData trianglesGeomData_l;
  trianglesGeomData_l.vertex = (float3*)vknBufferGetPointer(vertexBuffer_l);
  trianglesGeomData_l.index  = (int3*)vknBufferGetPointer(indexBuffer);
  trianglesGeomData_l.color  = {0,1,0};
  vknGeomSetData(trianglesGeom_l,&trianglesGeomData_l);
  
  // create and parametrize right geometry:
  VKNGeom trianglesGeom_r
    = vknGeomCreate(trianglesGeomType);
  vknTrianglesSetVertices(trianglesGeom_r,
                          NUM_VERTICES,
                          vknBufferGetPointer(vertexBuffer_r),
                          sizeof(float3));
  vknTrianglesSetIndices(trianglesGeom_r,
                         NUM_INDICES,
                         vknBufferGetPointer(indexBuffer),
                         sizeof(int3));
  TrianglesGeomData trianglesGeomData_r;
  trianglesGeomData_r.vertex = (float3*)vknBufferGetPointer(vertexBuffer_r);
  trianglesGeomData_r.index  = (int3*)vknBufferGetPointer(indexBuffer);
  trianglesGeomData_r.color  = {0,0,1};
  vknGeomSetData(trianglesGeom_r,&trianglesGeomData_r);
  
  // ------------------------------------------------------------------
  // the group/accel for that mesh
  // ------------------------------------------------------------------
  VKNGeom geoms[2] = { trianglesGeom_l, trianglesGeom_r };
  VKNAccel trianglesGroup
    = vknTrianglesAccelCreate(context,2,geoms);
  vknAccelBuild(trianglesGroup);
  VKNAccel world
    = vknInstanceAccelCreate(context,1,&trianglesGroup);
  vknAccelBuild(world);

  // ##################################################################
  // set miss and raygen program required for SBT
  // ##################################################################

  // -------------------------------------------------------
  // set up ray gen program
  // -------------------------------------------------------
  
  // ----------- create object  ----------------------------
  VKNRayGen rayGen
    = vknRayGenCreate(context,module,"simpleRayGen",
                      sizeof(RayGenData));

  // ----------- compute variable values  ------------------
  RayGenData rgData;

  // camera:
  float aspect = fbSize.x / float(fbSize.y);
  rgData.camera.pos = lookFrom;
  rgData.camera.dir_00
    = normalize(lookAt-lookFrom);
  rgData.camera.dir_du
    = cosFovy * aspect * normalize(cross(rgData.camera.dir_00,lookUp));
  rgData.camera.dir_dv
    = cosFovy * normalize(cross(rgData.camera.dir_du,rgData.camera.dir_00));
  rgData.camera.dir_00 -= 0.5f * rgData.camera.dir_du;
  rgData.camera.dir_00 -= 0.5f * rgData.camera.dir_dv;

  // frame buffer:
  VKNBuffer frameBuffer
    = vknDeviceBufferCreate(context,sizeof(int)*fbSize.x*fbSize.y,nullptr);
  rgData.fbPtr = (uint32_t*)vknBufferGetPointer(frameBuffer);
  rgData.fbSize = fbSize;

  // world to trace against:
  rgData.world = vknAccelGetTraversableHandle(world);
  // ----------- and set data  ----------------------------
  vknRayGenSetData(rayGen,&rgData);

#if 1
  // ##################################################################
  // create and set a miss program so SBT entry isn't emtpy
  // ##################################################################
  VKNMissProg missProg
    = vknMissProgCreate(context,module,"simpleMissProg",
                      sizeof(MissProgData));
#else
  VKNMissProg missProg = 0;
#endif
  
  // ##################################################################
  // build *SBT* required to trace the groups
  // ##################################################################
  // vknBuildPrograms(context);
  vknBuildPipeline(context);
  vknBuildSBT(context);

  // ##################################################################
  // now that everything is ready: launch it ....
  // ##################################################################

  LOG("launching ...");
  vknRayGenLaunch2D(rayGen,missProg,fbSize.x,fbSize.y,&dummy);

  LOG("done with launch, writing picture ...");
  const uint32_t *fb
    = (const uint32_t*)vknBufferMap(frameBuffer);
  assert(fb);
  stbi_write_png(outFileName,fbSize.x,fbSize.y,4,
                 fb,fbSize.x*sizeof(uint32_t));
  vknBufferUnmap(frameBuffer);
  LOG_OK("written rendered frame buffer to file "<<outFileName);
  // ##################################################################
  // and finally, clean up
  // ##################################################################

  LOG("destroying devicegroup ...");
  // vknContextDestroy(context);
  vknTerminate(vkn);

  LOG_OK("seems all went OK; app is done, this should be the last output ...");
}
