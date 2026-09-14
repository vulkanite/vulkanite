// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#include "vulkanite/vkn-common.h"

namespace vp {

  struct Context;
  struct Mesh;
  
  struct Group {
    Group(Context *context,
          const std::vector<Mesh *> &meshes);
    Context *const context;
    VKNAccel vkn = 0;
  };
  
}
