// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#include "vulkanite/Object.h"
#include "vulkanite/Context.h"

namespace vkn {

  Object::Object(Context *context)
    : device(context->device),
      context(context)
  {}
  
}
