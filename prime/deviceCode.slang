// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#include "deviceCode.h"

#define INT_MAX ((int32_t)0x7fffffff)

[[vk::push_constant]] PushConstants pc;

struct HitRef {
  int   primID;
  int   geomID;
  int   instID;
  float t;
};

bool less(HitRef a, HitRef b)
{
  if (a.t < b.t) return true;
  if (a.t > b.t) return false;

  if (a.primID < b.primID) return true;
  if (a.primID > b.primID) return false;
  
  if (a.geomID < b.geomID) return true;
  if (a.geomID > b.geomID) return false;
  
  if (a.instID < b.instID) return true;
  if (a.instID > b.instID) return false;

  return false;
}

bool greater(HitRef a, HitRef b)
{ return less(b,a); }

struct PRD {
  HitRef hurdle;
  struct {
    HitRef hit;
    float2 uv;
  } closestSoFar;
};

[shader("closesthit")] 
void TriangleMesh_CH(uniform TrianglesGeomData self,
                     inout PRD prd,
                     in float2 uv
                     )
{
}

inline HitRef currentHit(uniform TrianglesGeomData self)
{
  HitRef current;
  current.t = RayTCurrent();
  current.primID = PrimitiveIndex();
  current.instID = InstanceIndex();
  current.geomID = self.geomID;
  return current;
}
 
[shader("anyhit")] 
void TriangleMesh_AH(uniform TrianglesGeomData self,
                     inout PRD prd,
                     float2 bc)
{
  HitRef current = currentHit(self);
  if (current.t > prd.closestSoFar.hit.t) {
    // do NOT ignore, let pipeline accept this hit even though *we*
    // won't save it; this'll shorten the ray length.
    return;
  }
  
  if (!greater(current,prd.hurdle)) {
    IgnoreHit();
    return;
  }

  // at this point the current interseciton must be either closer or
  // same distance.
  if (less(current,prd.closestSoFar.hit)) {
    prd.closestSoFar.hit = current;
    prd.closestSoFar.uv  = bc;
    // we _did_ accept, but still have to reject at that distance,
    // so the pipeline will still call us with other hits at same
    // distance.
    IgnoreHit();
  }
}

[shader("miss")] 
void missProg(uniform MissProgData record,
                    inout PRD prd)
{ /* nothing to do */}


float nextLowerThan(float f)
{ return nextafter(f,-1.f); }

[shader("raygeneration")] 
void traceRays(uniform RayGenData self)
{
  int rayID = DispatchRaysIndex().x;
  if (rayID >= self.numRays) return;

  PrimeRay in_ray = self.rays[rayID];

  PRD prd;

  vkn::Ray ray;
  ray.origin = in_ray.org;
  ray.direction = in_ray.dir;
  
  if (self.traceMode == (int)VPTraceMode::VP_TRACE_MODE_FIRST_HIT) {
    // any hit that could possibly clear this hurdle would have to
    // have t > tmin
    ray.tMin          = in_ray.tMin;
    prd.hurdle.t      = in_ray.tMin;
    prd.hurdle.primID = INT_MAX;
    prd.hurdle.geomID = INT_MAX;
    prd.hurdle.instID = INT_MAX;
  } else {
    PrimeHit prev_hit = self.hits[rayID];
    // last hit become new hurdle
    prd.hurdle.t      = prev_hit.t;
    prd.hurdle.primID = prev_hit.primID;
    prd.hurdle.geomID = prev_hit.geomID;
    prd.hurdle.instID = prev_hit.instID;
    ray.tMin = nextLowerThan(prd.hurdle.t);
  }
  ray.tMax = in_ray.tMax;
  
  prd.closestSoFar.hit.t      = in_ray.tMax;
  prd.closestSoFar.hit.primID = INT_MAX;
  prd.closestSoFar.hit.geomID = INT_MAX;
  prd.closestSoFar.hit.instID = INT_MAX;
  
  traceRay(self.world,
           ray,
           prd);
  self.hits[rayID].primID = prd.closestSoFar.hit.primID;
  self.hits[rayID].instID = prd.closestSoFar.hit.instID;
  self.hits[rayID].geomID = prd.closestSoFar.hit.geomID;
  self.hits[rayID].t      = prd.closestSoFar.hit.t;
  self.hits[rayID].u      = prd.closestSoFar.uv.x;
  self.hits[rayID].v      = prd.closestSoFar.uv.y;
  
  self.hits[rayID] = self.hits[rayID];
}

