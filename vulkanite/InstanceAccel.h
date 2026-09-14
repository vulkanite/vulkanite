// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "vulkanite/Accel.h"

namespace vkn {
  struct Context;

  struct InstanceAccel : Accel {
    typedef std::shared_ptr<InstanceAccel> SP;

    InstanceAccel(Context *context,
                  const std::vector<GeomAccel::SP> &accels);
    ~InstanceAccel() override;

    std::string toString() override;

    void setTransformsHost(VKNFloat4x3 *transforms);
    
    void build() override;

    DeviceBuffer::SP asHandlesBuffer;
    DeviceBuffer::SP instanceBuffer;
    const std::vector<GeomAccel::SP> accels;
    std::vector<VKNFloat4x3> hostTransforms;
  };
  
}
