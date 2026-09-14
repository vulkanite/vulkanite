// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#include "vulkanite/vkn-common.h"

namespace vkn {
  const char *getErrorString(VkResult result)
  {
    switch(result) {
    case VK_ERROR_UNKNOWN:
      return "unknown error";
    case VK_SUCCESS: 
      return "no error";
    case VK_NOT_READY: 
      return "not ready";
    case VK_TIMEOUT: 
      return "timeout";
    case VK_EVENT_SET: 
      return "event set";
    case VK_EVENT_RESET: 
      return "event reset";
    case VK_INCOMPLETE: 
      return "incomplete";
    case VK_ERROR_OUT_OF_HOST_MEMORY: 
      return "out of host memory";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY: 
      return "out of device memory";
    case VK_ERROR_INITIALIZATION_FAILED: 
      return "initialization failed";
    case VK_ERROR_DEVICE_LOST: 
      return "device lost";
    case VK_ERROR_MEMORY_MAP_FAILED: 
      return "memory map failed";
    case VK_ERROR_LAYER_NOT_PRESENT: 
      return "layer not present";
    case VK_ERROR_EXTENSION_NOT_PRESENT: 
      return "extension not present";
    case VK_ERROR_FEATURE_NOT_PRESENT: 
      return "feature not present";
    case VK_ERROR_INCOMPATIBLE_DRIVER: 
      return "incompatible driver";
    case VK_ERROR_TOO_MANY_OBJECTS: 
      return "too many objects";
    case VK_ERROR_FORMAT_NOT_SUPPORTED: 
      return "format not supported";
    case VK_ERROR_SURFACE_LOST_KHR: 
      return "surface lost";
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: 
      return "window in use";
    case VK_SUBOPTIMAL_KHR: 
      return "suboptimal";
    case VK_ERROR_OUT_OF_DATE_KHR: 
      return "out of date";
    case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: 
      return "incompatible display";
    case VK_ERROR_VALIDATION_FAILED_EXT: 
      return "validation failed";
    case VK_ERROR_INVALID_SHADER_NV: 
      return "invalid shader";
    case VK_ERROR_OUT_OF_POOL_MEMORY: 
      return "out of pool memory";
    };
    throw std::runtime_error("un-handled error code #"+std::to_string((int)result));
  }

  /*! check if result == VKN_SUCCESS, and if so, do
    nothing. Otherwise, throw an exception (or later on, maybe
    some other form of logging/error handling?) based on where the
    error happened, what it is about, and what the error code
    was */
  void checkError(const std::string &where,
                  VkResult result,
                  const std::string &message)
  {
    if (result == VK_SUCCESS) return;

    std::string error = "!vkn @"+where+"\n"+
                             "!vkn fatal error '"+message+"'\n"+
      "!vkn vulkan error: "+getErrorString(result);
    ::detail::vknRaise_impl(error);
    // throw std::runtime_error();
  }
    
};

