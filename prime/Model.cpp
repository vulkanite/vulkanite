// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#include "prime/Model.h"
#include "prime/Group.h"
#include "prime/Context.h"

namespace vp {

  Model::Model(Context *context,
               const std::vector<Group *> &groups,
               const float *xfms)
    : context(context)
  {
    std::vector<VKNAccel> groupAccels;
    for (auto group : groups)
      groupAccels.push_back(group->vkn);
    this->vkn = vknInstanceAccelCreate(context->vkn,
                                   groups.size(),
                                   groupAccels.data());
    vknAccelBuild(this->vkn);
    vknBuildSBT(context->vkn);
  }
  
}
