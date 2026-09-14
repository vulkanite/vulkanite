// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#include "prime/Group.h"
#include "prime/Mesh.h"
#include "prime/Context.h"

namespace vp {

  Group::Group(Context *context,
               const std::vector<Mesh *> &meshes)
    : context(context)
  {
    std::vector<VKNGeom> geoms;
    for (auto mesh : meshes)
      geoms.push_back(mesh->vkn);
    this->vkn
      = vknTrianglesAccelCreate(context->vkn,
                                geoms.size(),
                                geoms.data());
    vknAccelBuild(this->vkn);
  }
  
}
