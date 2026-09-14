# SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
# SPDX-License-Identifier: Apache-2.0

# vulkanite/vulkanite/ : defines 'vulkanite', the main vulkanite library,
# similar to OWL, GPRT, or the barney rtcore abstraction

include(cmake/vulkanite_embed_devicecode.cmake)

option(VKN_VALIDATION_ENABLED "Enable Validation Layer?" ON)
configure_file(
  ${PROJECT_SOURCE_DIR}/vulkanite/include/vulkanite/vulkanite.h
  ${CMAKE_CURRENT_BINARY_DIR}/include/vulkanite/vulkanite.h
)
configure_file(
  ${PROJECT_SOURCE_DIR}/vulkanite/include/vulkanite/vulkanite_device.h
  ${CMAKE_CURRENT_BINARY_DIR}/include/vulkanite/vulkanite_device.h
)

add_library(vulkanite-config INTERFACE)
target_include_directories(vulkanite-config INTERFACE
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/3rdParty>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include/>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(vulkanite-config INTERFACE
  $<BUILD_INTERFACE:glm::glm>
)

set(VULKANITE_FILES
  driver/common.h
  driver/common.cpp
  PhysicalDevice.h
  PhysicalDevice.cpp

  RangeAllocator.h
  RangeAllocator.cpp
  
  Buffer.h
  Buffer.cpp
  Sampler.h
  Sampler.cpp
  Image.h
  Image.cpp

  SBTObject.h
  SBTObject.cpp
  
  driver/spirv.h
  #  driver/spirv_reflect.h
  # driver/spirv_reflect.cpp
  
  LogicalDevice.h
  LogicalDevice.cpp
  DriverInstance.h
  DriverInstance.cpp
  ShaderModule.h
  ShaderModule.cpp
  ShaderProgram.h
  ShaderProgram.cpp
  RayGen.h
  RayGen.cpp
  MissProg.h
  MissProg.cpp
  GeomType.h
  GeomType.cpp
  RTPipeline.h
  RTPipeline.cpp
  Compute.h
  Compute.cpp

  Triangles.h
  Triangles.cpp
  UserGeom.h
  UserGeom.cpp
  InstanceAccel.h
  InstanceAccel.cpp
  DescriptorPool.h
  DescriptorPool.cpp

  Accel.h
  Accel.cpp
  
  Object.h
  Object.cpp
  Context.h
  Context.cpp
  vkn-common.h
  vkn-common.cpp
  
  vulkanite.cpp
  vulkanite.cpp
)




find_package(Vulkan REQUIRED)

target_include_directories(vulkanite-config
  INTERFACE
  ${Vulkan_INCLUDE_DIR}
)

target_include_directories(vulkanite-config
  INTERFACE
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}>
)


add_library(vulkanite STATIC ${VULKANITE_FILES})
target_link_libraries(vulkanite PUBLIC
  $<BUILD_INTERFACE:vulkanite-config>
  $<BUILD_INTERFACE:vma>
  ${Vulkan_LIBRARY}
)
target_include_directories(vulkanite PUBLIC
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}>
  )
set_target_properties(vulkanite
  PROPERTIES
  POSITION_INDEPENDENT_CODE    ON
)

add_library(vulkanite::vulkanite ALIAS vulkanite)
