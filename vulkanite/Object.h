// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "vulkanite/LogicalDevice.h"

namespace vkn {
  struct Context;

  struct Object : public std::enable_shared_from_this<Object> {
    typedef std::shared_ptr<Object> SP;

    Object(Context *context);
    virtual ~Object() {}

    virtual std::string toString() = 0;
    
    template<typename T> inline std::shared_ptr<T> asSP() 
    { return std::dynamic_pointer_cast<T>(shared_from_this()); }
    template<typename T> inline std::shared_ptr<const T> asSP() const
    { return std::dynamic_pointer_cast<T>(shared_from_this()); }

    LogicalDevice::SP const device;
    Context          *const context;
  };
  
}
