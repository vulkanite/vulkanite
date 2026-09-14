// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#pragma once

#ifdef __VKN_DEVICE__
# include <vulkanite/vulkanite_device.h>
#else
# include <vulkanite/vulkanite.h>
# include <samples/samples-vulkanite-math.h>
#endif


/* variables for the triangle mesh geometry */
struct TrianglesGeomData
{
  /*! base color we use for the entire mesh */
  float3  color;
  /*! array/buffer of vertex indices */
  int3   *index;
  /*! array/buffer of vertex positions */
  float3 *vertex;
};

struct MissProg {
  int foo;
};

/* variables for the ray generation program */
struct RayGenData
{
  uint32_t *fbPtr;
  int2  fbSize;
  VKNTraversableHandle world;

  struct {
    float3 pos;
    float3 dir_00;
    float3 dir_du;
    float3 dir_dv;
  } camera;
};

/* variables for the miss program */
struct MissProgData
{
  float3  color0;
  float3  color1;
};

