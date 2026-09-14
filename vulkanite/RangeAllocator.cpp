// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#include "vulkanite/RangeAllocator.h"

namespace vkn {

  /*! allocate 'size' consecutive SBT entries, and return index of
      first of those */
  int RangeAllocator::alloc(size_t size)
  {
    for (size_t i=0;i<freedRanges.size();i++) {
      if (freedRanges[i].size >= size) {
        size_t where = freedRanges[i].begin;
        if (freedRanges[i].size == size)
          freedRanges.erase(freedRanges.begin()+i);
        else {
          freedRanges[i].begin += size;
            freedRanges[i].size  -= size;
        }
        return (int)where;
      }
    }
    size_t where = maxAllocedID;
    maxAllocedID+=size;
    assert(maxAllocedID == size_t(int(maxAllocedID)));
    return (int)where;
  }

  /*! a given group has died, and tells us to release given range
      (starting at begin, with 'siez' elements', to be re-used when
      appropriate */
  void RangeAllocator::release(size_t begin, size_t size)
  {
    for (size_t i=0;i<freedRanges.size();i++) {
      if (freedRanges[i].begin+freedRanges[i].size == begin) {
        begin -= freedRanges[i].size;
        size  += freedRanges[i].size;
        freedRanges.erase(freedRanges.begin()+i);
        release(begin,size);
        return;
      }
      if (begin+size == freedRanges[i].begin) {
        size  += freedRanges[i].size;
        freedRanges.erase(freedRanges.begin()+i);
        release(begin,size);
        return;
      }
    }
    if (begin+size == maxAllocedID) {
      maxAllocedID -= size;
      return;
    }
    // could not merge with any existing range: add new one
    freedRanges.push_back({begin,size});
  }

} // ::vkn
