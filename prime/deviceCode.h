// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#pragma once

#if __VKN_DEVICE__
# include <vulkanite/vulkanite_device.h>

typedef enum _VPTraceMode {
  VP_TRACE_MODE_FIRST_HIT,
  VP_TRACE_MODE_NEXT_HIT
} VPTraceMode;
#else
# include <vulkanite/vulkanite.h>
# include <vulkanite/vkn-common.h>
# include <prime/vkn-prime-common.h>
# include <vulkanite/prime.h>
#endif

using namespace vkn;

struct PrimeRay {
  float3 org;
  float  tMin;
  float3 dir;
  float  tMax;
};

struct PrimeHit {
  int primID;
  int geomID;
  int instID;
  float t;
  float u;
  float v;
};

/* variables for the triangle mesh geometry */
struct TrianglesGeomData
{
  uint32_t geomID;
};

/* variables for the miss program */
struct MissProgData
{
  int ignoreJustNotEmpty;
};

/* variables for the ray generation program */
struct RayGenData
{
  PrimeRay *rays;
  int      *rayIDs;
  PrimeHit *hits;
  int      *hitIDs;
  int                  traceMode;
  VKNTraversableHandle world;
  int                  numRays;
};

struct PushConstants
{
  // PrimeRay *rays;
  // int      *rayIDs;
  // PrimeHit *hits;
  // int      *hitIDs;
  // uint64_t traceMode;
  // VKNTraversableHandle world;
  // int                  numRays;
};

  
