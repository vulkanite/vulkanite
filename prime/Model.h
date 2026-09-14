// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#include "prime/vkn-prime-common.h"

namespace vp {

  struct Context;
  struct Group;
  
  struct Model {
    Model(Context *context,
          const std::vector<Group *> &groups,
          const float *xfms);
    Context *const context;
    VKNAccel vkn = 0;
  };
  
}
