// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <sys/types.h>
#include <stdint.h>

#if defined(_MSC_VER)
# if defined(owl_prime_static_STATIC)
#  define VP_INTERFACE /* nothing */
# elif defined(owl_prime_EXPORTS)
#  define VP_INTERFACE __declspec(dllexport)
# else
#  define VP_INTERFACE __declspec(dllimport)
# endif
#elif defined(__clang__) || defined(__GNUC__)
#  define VP_INTERFACE __attribute__((visibility("default")))
#else
#  define VP_INTERFACE
#endif

#ifdef __cplusplus
#  define VP_API extern "C" VP_INTERFACE
#  define VP_IF_CPP(a) a
#else
#  define VP_API /* nothing */ VP_INTERFACE
#  define VP_IF_CPP(a) /* ignore */
#endif

struct VPfloat3 { float x,y,z; };
struct VPfloat4 { float x,y,z,w; };

/*! defines data layout for a input ray for those functions that
  operate on arrays of rays in AoS layout */
struct VPRay {
  VPfloat3 origin;
  float    tMin;
  VPfloat3 direction;
  float    tMax;
};

struct VPTransform {
  struct {
    VPfloat3 vx;
    VPfloat3 vy;
    VPfloat3 vz;
  } l;
  VPfloat3 p;
};

/*! sruct describing a ray's hit; when user supplied device-side
  arrays this array has to be 16-byte aligned; for host-side data
  arrays that get automatically uploaded this is not required. */
struct VPHit {
  /*! the (user-provided) 64-bit data value that is associated with the
    given mesh that the intersected primitmive was in */
  uint64_t geomDataValue;
  
  /*! the 'primitive' within a given mesh. a value of -1 here will
    indicate that nothing was hit */
  int primID;
  
  /*! (linear) ID of the instance in the array of instances */
  int instID;
  
  /*! distance to, and barycentric surface coordinates of the hit
    point */
  float t, u, v;
};

/*! the VPContext is the root context object for all of primer; it
  manages creation of new groups, models, meshes, etc; baesd on the
  *type* of context it may do this in various different ways */
typedef struct _VPContext  *VPContext;

/*! a VPModel is a object that contains one or more geometries
  (optionally using groups and/or instances), and that rays can be
  traced against */
typedef struct _VPModel    *VPModel;

/*! groups can be used to group one or more geometries together for
  later instantiation. All geometries in a group have to be of the
  same type (ie, you can have a group with multiple meshes, and
  another for multiple spheres geometrys; but you can not have one
  group that contains both meshes and sphere geometries. */
typedef struct _VPGroup    *VPGroup;

/*! a "geometry" that contains one of more geometric primitives like
  spheres, triangles, etc. Note geomtries are "virtual" in the sense
  that this geom could be either a "spheres" geometry, a "boxes"
  geometry, depending on how it was created */
typedef struct _VPGeom     *VPGeom;

typedef enum {
  VP_TRACE_MODE_FIRST_HIT = 0,
  VP_TRACE_MODE_NEXT_HIT
} VPTraceMode;

#ifndef __cplusplus
extern "C" {
#endif

  VP_API VPContext vpContextCreate(/*! which GPU to use, '0' being the first
                                       *  GPU, '1' the second, etc. '-1' means
                                       *  'use host CPU only' */
                                      int32_t gpuToUse);
  
  VP_API void  vpContextDestroy(VPContext context);


  // ==================================================================
  // (Trangle-)Meshes: triangle meshes are made up of one or more
  // triangles, specified through a set of vertices and a set of
  // vertex indices. For convenience we also allow for creating meshes
  // from individual triangles, and/or various different memory
  // layouts for specifying these vertices and/or indices.
  // ==================================================================
  VP_API VPGeom vpMeshCreate(VPContext context,
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
                                size_t sizeOfIndexStructInBytes);
  
  // ==================================================================
  // model creation. Models can contain anything from a single
  // geometry to multiple geometries in multiple groups, and possibly
  // intances thereof. Internally all models will be organized with
  // instances and groups; but to allow users that do not use
  // instances or groups (or even want to know what those are!) there
  // are also convenience functions to create a model from a single
  // geometry, or from a "plain" list of geometries.
  // ==================================================================
  
  VP_API
  VPGroup vpGroupCreate(VPContext _context,
                          VPGeom *geoms,
                          int numGeoms);
  VP_API
  VPModel vpModelCreate(VPContext    context,
                        VPGroup     *groups,
                        VPTransform *xfms,
                        int          numInstances);

  // ==================================================================
  // how to actually trace (arrays) of rays and get (arrays) of hits
  // ==================================================================
  
  
  /*! trace all the rays in the given array of input rays, and write
   *  results into the given array of output rays. Unlike
   *  vpTraceAsync, this call is synchronous, so will block the
   *  calling thread until all rays are traced and all hits have been
   *  written. Input and output arrays have to remain valid and
   *  unmodified while teh call is active, but can be modified or
   *  released the moment this call returns */
  VP_API void vpTrace(/*! the model to trace into */
                        VPModel model,
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
                        VPTraceMode traceMode);

  VP_API void *vpMalloc(VPContext _context,
                        size_t size);
#ifndef __cplusplus
}
#endif
