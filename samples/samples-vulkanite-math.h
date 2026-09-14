// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
    layer of classes that makes C++ 'understand' slang-style types like
    float3, int2, etc, including some operations on that (like
    normalize()) */
#pragma once

#if __VKN_DEVICE__
#  error "this file should be included only on the host, to make it 'understand' slang math/vector classes"
#endif

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include <cmath>

using float2 = glm::vec2;
using float3 = glm::vec3;
using float4 = glm::vec4;
using int2   = glm::ivec2;
using int3   = glm::ivec3;
using int4   = glm::ivec4;


inline float dot(float3 a, float3 b)
{ return a.x*b.x + a.y*b.y + a.z*b.z; }


inline float3 cross(float3 a, float3 b)
{ return float3(a.y*b.z-a.z*b.y,
                a.z*b.x-a.x*b.z,
                a.x*b.y-a.y*b.x); }


inline float3 normalize(float3 v)
{ return v*(1.f/std::sqrt(dot(v,v))); }
