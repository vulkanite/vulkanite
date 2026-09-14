// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "vulkanite/vulkanite.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <set>
#include <string>
#include <memory>
#include <iostream>
#ifndef _USE_MATH_DEFINES
#  define _USE_MATH_DEFINES
#endif
// #include <math.h> // using cmath causes issues under Windows

#include <iostream>
#include <stdexcept>
#include <memory>
#include <string>
#include <algorithm>
#include <sstream>
#ifdef __GNUC__
#include <execinfo.h>
#include <sys/time.h>
#endif

#include <cmath>
#include <cfloat>
#include <cstring>
#include <cstdio>
#include <cassert>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/fwd.hpp>
#include <stack>
#include <vector>
#include <map>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#endif

#if !defined(WIN32)
#include <signal.h>
#endif

#if defined(_MSC_VER)
#  define VKN_DLL_EXPORT __declspec(dllexport)
#  define VKN_DLL_IMPORT __declspec(dllimport)
#elif defined(__clang__) || defined(__GNUC__)
#  define VKN_DLL_EXPORT __attribute__((visibility("default")))
#  define VKN_DLL_IMPORT __attribute__((visibility("default")))
#else
#  define VKN_DLL_EXPORT
#  define VKN_DLL_IMPORT
#endif


// -----------------------------------------------------------------------------
// PRINT/PING
// -----------------------------------------------------------------------------

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#ifndef PRINT
#define PRINT(var) std::cout << #var << "=" << var << std::endl;
#ifdef _WIN32
# define __PRETTY_FUNCTION__ TOSTRING(__LINE__) "::" TOSTRING(__FUNCTION__)
# define PING                                                           \
  std::cout << __FILE__ << "::" << __LINE__ << ": " << __FUNCTION__     \
  << std::endl;
#else
# define PING                                                           \
  std::cout << __FILE__ << "::" << __LINE__ << ": " << __PRETTY_FUNCTION__ \
  << std::endl;
#endif
#endif

// -----------------------------------------------------------------------------
// the usual owl colors, owlraise, etc
// -----------------------------------------------------------------------------

#ifdef __GNUC__
#define MAYBE_UNUSED __attribute__((unused))
#else
#define MAYBE_UNUSED
#endif

namespace detail {
  inline static std::string backtrace()
  {
#ifdef __GNUC__
    static const int max_frames = 16;

    void* buffer[max_frames] = { 0 };
    int cnt = ::backtrace(buffer,max_frames);

    char** symbols = backtrace_symbols(buffer,cnt);

    if (symbols) {
      std::stringstream str;
      for (int n = 1; n < cnt; ++n) // skip the 1st entry (address of this function)
        {
          str << symbols[n] << '\n';
        }
      free(symbols);
      return str.str();
    }
    return "";
#else
    return "not implemented yet";
#endif
  }

  inline void vknRaise_impl(std::string str)
  {
    fprintf(stderr,"%s\n",str.c_str());
#ifdef WIN32
    if (IsDebuggerPresent())
      DebugBreak();
    else
      throw std::runtime_error(str);
#else
#ifndef NDEBUG
    std::string bt = ::detail::backtrace();
    fprintf(stderr,"%s\n",bt.c_str());
#endif
    raise(SIGINT);
#endif
  }
}

#define VKN_RAISE(MSG) ::detail::vknRaise_impl(MSG);


#define VKN_NOTIMPLEMENTED throw std::runtime_error(std::string(__PRETTY_FUNCTION__)+" not implemented")

#ifdef WIN32
# define VKN_TERMINAL_RED ""
# define VKN_TERMINAL_GREEN ""
# define VKN_TERMINAL_LIGHT_GREEN ""
# define VKN_TERMINAL_YELLOW ""
# define VKN_TERMINAL_BLUE ""
# define VKN_TERMINAL_LIGHT_BLUE ""
# define VKN_TERMINAL_RESET ""
# define VKN_TERMINAL_DEFAULT VKN_TERMINAL_RESET
# define VKN_TERMINAL_BOLD ""

# define VKN_TERMINAL_MAGENTA ""
# define VKN_TERMINAL_LIGHT_MAGENTA ""
# define VKN_TERMINAL_CYAN ""
# define VKN_TERMINAL_LIGHT_RED ""
#else
# define VKN_TERMINAL_RED "\033[0;31m"
# define VKN_TERMINAL_GREEN "\033[0;32m"
# define VKN_TERMINAL_LIGHT_GREEN "\033[1;32m"
# define VKN_TERMINAL_YELLOW "\033[1;33m"
# define VKN_TERMINAL_BLUE "\033[0;34m"
# define VKN_TERMINAL_LIGHT_BLUE "\033[1;34m"
# define VKN_TERMINAL_RESET "\033[0m"
# define VKN_TERMINAL_DEFAULT VKN_TERMINAL_RESET
# define VKN_TERMINAL_BOLD "\033[1;1m"

# define VKN_TERMINAL_MAGENTA "\e[35m"
# define VKN_TERMINAL_LIGHT_MAGENTA "\e[95m"
# define VKN_TERMINAL_CYAN "\e[36m"
# define VKN_TERMINAL_LIGHT_RED "\033[1;31m"
#endif

#ifdef _MSC_VER
# define VKN_ALIGN(alignment) __declspec(align(alignment)) 
#else
# define VKN_ALIGN(alignment) __attribute__((aligned(alignment)))
#endif



// -----------------------------------------------------------------------------
// VK_CALL() - basically owl's CUDA_CALL()
// -----------------------------------------------------------------------------

namespace vkn {
  using namespace glm;
  const char *getErrorString(VkResult rc);

  inline size_t divRoundUp(size_t a, size_t b) { return (a+b-1)/b; }
  inline uint32_t divRoundUp(uint32_t a, uint32_t b) { return (a+b-1)/b; }
  inline int divRoundUp(int a, int b) { return (a+b-1)/b; }
  
  inline void *alignAddress(void *ptr, size_t alignment)
  {
    uint64_t v = (uint64_t)ptr;
    v = (v+alignment-1) & ~(alignment-1);
    return (void *)v;
  }
  
  
#ifdef __WIN32__
#  define vkn_snprintf sprintf_s
#else
#  define vkn_snprintf snprintf
#endif
  
  
  inline std::string prettyDouble(const double val) {
    const double absVal = abs(val);
    char result[1000];

    if      (absVal >= 1e+18f) vkn_snprintf(result,1000,"%.1f%c",float(val/1e18f),'E');
    else if (absVal >= 1e+15f) vkn_snprintf(result,1000,"%.1f%c",float(val/1e15f),'P');
    else if (absVal >= 1e+12f) vkn_snprintf(result,1000,"%.1f%c",float(val/1e12f),'T');
    else if (absVal >= 1e+09f) vkn_snprintf(result,1000,"%.1f%c",float(val/1e09f),'G');
    else if (absVal >= 1e+06f) vkn_snprintf(result,1000,"%.1f%c",float(val/1e06f),'M');
    else if (absVal >= 1e+03f) vkn_snprintf(result,1000,"%.1f%c",float(val/1e03f),'k');
    else if (absVal <= 1e-12f) vkn_snprintf(result,1000,"%.1f%c",float(val*1e15f),'f');
    else if (absVal <= 1e-09f) vkn_snprintf(result,1000,"%.1f%c",float(val*1e12f),'p');
    else if (absVal <= 1e-06f) vkn_snprintf(result,1000,"%.1f%c",float(val*1e09f),'n');
    else if (absVal <= 1e-03f) vkn_snprintf(result,1000,"%.1f%c",float(val*1e06f),'u');
    else if (absVal <= 1e-00f) vkn_snprintf(result,1000,"%.1f%c",float(val*1e03f),'m');
    else vkn_snprintf(result,1000,"%f",(float)val);

    return result;
  }
  

  /*! return a nicely formatted number as in "3.4M" instead of
    "3400000", etc, using mulitples of thousands (K), millions
    (M), etc. Ie, the value 64000 would be returned as 64K, and
    65536 would be 65.5K */
  inline std::string prettyNumber(const size_t s)
  {
    char buf[1000];
    if (s >= (1000LL*1000LL*1000LL*1000LL)) {
      vkn_snprintf(buf, 1000,"%.2fT",s/(1000.f*1000.f*1000.f*1000.f));
    } else if (s >= (1000LL*1000LL*1000LL)) {
      vkn_snprintf(buf, 1000, "%.2fG",s/(1000.f*1000.f*1000.f));
    } else if (s >= (1000LL*1000LL)) {
      vkn_snprintf(buf, 1000, "%.2fM",s/(1000.f*1000.f));
    } else if (s >= (1000LL)) {
      vkn_snprintf(buf, 1000, "%.2fK",s/(1000.f));
    } else {
      vkn_snprintf(buf,1000,"%zi",s);
    }
    return buf;
  }

  /*! return a nicely formatted number as in "3.4M" instead of
    "3400000", etc, using mulitples of 1024 as in kilobytes,
    etc. Ie, the value 65534 would be 64K, 64000 would be 63.8K */
  inline std::string prettyBytes(const size_t s)
  {
    char buf[1000];
    if (s >= (1024LL*1024LL*1024LL*1024LL)) {
      vkn_snprintf(buf, 1000,"%.2fT",s/(1024.f*1024.f*1024.f*1024.f));
    } else if (s >= (1024LL*1024LL*1024LL)) {
      vkn_snprintf(buf, 1000, "%.2fG",s/(1024.f*1024.f*1024.f));
    } else if (s >= (1024LL*1024LL)) {
      vkn_snprintf(buf, 1000, "%.2fM",s/(1024.f*1024.f));
    } else if (s >= (1024LL)) {
      vkn_snprintf(buf, 1000, "%.2fK",s/(1024.f));
    } else {
      vkn_snprintf(buf,1000,"%zi",s);
    }
    return buf;
  }
  
  inline double getCurrentTime()
  {
#ifdef _WIN32
    SYSTEMTIME tp; GetSystemTime(&tp);
    /*
      Please note: we are not handling the "leap year" issue.
    */
    size_t numSecsSince2020
      = tp.wSecond
      + (60ull) * tp.wMinute
      + (60ull * 60ull) * tp.wHour
      + (60ull * 60ul * 24ull) * tp.wDay
      + (60ull * 60ul * 24ull * 365ull) * (tp.wYear - 2020);
    return double(numSecsSince2020 + tp.wMilliseconds * 1e-3);
#else
    struct timeval tp; gettimeofday(&tp,nullptr);
    return double(tp.tv_sec) + double(tp.tv_usec)/1E6;
#endif
  }

  inline bool hasSuffix(const std::string &s, const std::string &suffix)
  {
    return s.substr(s.size()-suffix.size()) == suffix;
  }
    
  const char *getErrorString(VkResult rc);
  
  /*! check if result == VKN_SUCCESS, and if so, do
    nothing. Otherwise, throw an exception (or later on, maybe
    some other form of logging/error handling?) based on where the
    error happened, what it is about, and what the error code
    was */
  void checkError(const std::string &where,
                  VkResult result,
                  const std::string &message);
    
#define VK_CHECK(call,msg)                              \
  {                                                     \
    VkResult result = call;                             \
    ::vkn::checkError(__PRETTY_FUNCTION__,              \
                      result,msg);                      \
  }
    
#define VK_CALL(call,msg) VK_CHECK(vk##call,msg)
#define VK_DEVICE_CALL(call,msg) VK_CHECK(device->vk##call,msg)
}

