# SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 4.0.0 FATAL_ERROR)
#cmake_minimum_required(VERSION 3.28)

cmake_policy(SET CMP0104 NEW)
cmake_policy(SET CMP0079 NEW)
cmake_policy(SET CMP0076 NEW)
cmake_policy(SET CMP0063 NEW)

set(VKN_VERSION_MAJOR 0)
set(VKN_VERSION_MINOR 8)
set(VKN_VERSION_PATCH 0)

#set(CMAKE_BUILD_TYPE_INIT "Release")
set(CMAKE_BUILD_TYPE_INIT "Debug")
set(VKN_VERSION ${VKN_VERSION_MAJOR}.${VKN_VERSION_MINOR}.${VKN_VERSION_PATCH})
project(vulkanite VERSION ${VKN_VERSION} LANGUAGES C CXX)

set(VULKANITE_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type")
endif()
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
  "Debug" "Release" "MinSizeRel" "RelWithDebInfo")


include(GNUInstallDirs)
set(INSTALL_DESTINATION
  ${CMAKE_INSTALL_LIBDIR}/cmake/vulkanite-${VKN_VERSION}
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
  "${PROJECT_SOURCE_DIR}/vulkanite/cmake/vulkaniteConfig.cmake.in"
  "${PROJECT_BINARY_DIR}/vulkaniteConfig.cmake"
  INSTALL_DESTINATION
  ${INSTALL_DESTINATION}
)

write_basic_package_version_file(
  "vulkaniteConfigVersion.cmake"
  VERSION ${VKN_VERSION}
  COMPATIBILITY SameMajorVersion
)

add_subdirectory(3rdParty/vma)
add_subdirectory(3rdParty/glm)
add_subdirectory(vulkanite)

option(VKN_BUILD_SAMPLES "Build Samples?" OFF)

if (VKN_BUILD_SAMPLES)
  include(vulkanite/cmake/vulkanite_embed_devicecode.cmake)
  add_subdirectory(apps)
  add_subdirectory(samples)
  add_subdirectory(prime)
endif()

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/vulkaniteConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/vulkaniteConfigVersion.cmake
  ${PROJECT_SOURCE_DIR}/vulkanite/cmake/vulkanite_embed_devicecode.cmake
  ${PROJECT_SOURCE_DIR}/vulkanite/cmake/vulkanite_bin2c.cmake
  DESTINATION
  ${INSTALL_DESTINATION}
  )
  
install(TARGETS vulkanite vulkanite-config 
  EXPORT vulkanite-config
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  NAMELINK_SKIP
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  NAMELINK_SKIP
)
install(EXPORT vulkanite-config
  DESTINATION ${INSTALL_DESTINATION}
  NAMESPACE vulkanite::
)
install(
  FILES
  # preconfigured:
  ${CMAKE_CURRENT_BINARY_DIR}/vulkanite/include/vulkanite/vulkanite.h
  ${CMAKE_CURRENT_BINARY_DIR}/vulkanite/include/vulkanite/vulkanite_device.h

#  ${CMAKE_CURRENT_LIST_DIR}/vulkanite/include/vulkanite/vulkanite_device.h
  DESTINATION
  ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}/vulkanite/
)

    
