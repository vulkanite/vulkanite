// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "vulkanite/Object.h"

namespace vkn {
  
  struct Context;
  struct RTPipeline;
  /*! tracks which ID regions in the SBT have already been used -
    newly created groups allocate ranges of IDs in the SBT (to allow
    its geometries to be in successive SBT regions), and this struct
    keeps track of whats already used, and what is available */
  struct RangeAllocator {
    int alloc(size_t size);
    void release(size_t begin, size_t size);
    size_t maxAllocedID = 0;
  private:
    struct FreedRange {
      size_t begin;
      size_t size;
    };
    std::vector<FreedRange> freedRanges;
  };

} // ::vkn
