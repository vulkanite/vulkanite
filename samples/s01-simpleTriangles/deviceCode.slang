// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#include "deviceCode.h"

struct PushConstants {
  float3 v;
};

[[vk::push_constant]] PushConstants pc;

struct PRD {
  float3 color;
  bool   dbg;
};
  
[shader("closesthit")] 
void TriangleMesh_CH(uniform TrianglesGeomData self,
                     inout PRD prd,
                     in float2 uv
                     )
{
  if (prd.dbg)
    printf("CH!\n");
  prd.color = float3(uv.x,uv.y,1-uv.x-uv.y);
}

[shader("anyhit")] 
void TriangleMesh_AH(uniform TrianglesGeomData self,
                     inout PRD prd,
                     in float2 bc
                     )
{
}

[shader("miss")] 
void simpleMissProg(uniform MissProgData record, inout PRD prd)
{
}


[shader("raygeneration")] 
void simpleRayGen(uniform RayGenData self)
{
  uint2 pixelID    = DispatchRaysIndex().xy;
  uint2 launchDims = DispatchRaysDimensions().xy;

  PRD prd;
  prd.color = float3(0,0,0);
  prd.dbg  = all(pixelID == launchDims/2);
  const float2 screen = (float2(pixelID)+float2(.5f)) / float2(self.fbSize);
  vkn::Ray ray;
  ray.origin    
    = self.camera.pos;
  ray.direction 
    = normalize(self.camera.dir_00
                + screen.x * self.camera.dir_du
                + screen.y * self.camera.dir_dv);
  ray.tMin = 1e-4f;
  ray.tMax = 1e+4f;
  
  if (prd.dbg)
    printf("ray %f %f %f : %f %f %f\n",
           ray.origin.x,
           ray.origin.y,
           ray.origin.z,
           ray.direction.x,
           ray.direction.y,
           ray.direction.z);

  vkn::traceRay(self.world,
                ray,
                prd);
  
  const int      fbOfs = pixelID.x+self.fbSize.x*pixelID.y;
  const uint32_t bgra  = vkn::make_bgra(prd.color);
  self.fbPtr[fbOfs]    = bgra;
}

