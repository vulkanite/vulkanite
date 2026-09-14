# SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
# SPDX-License-Identifier: Apache-2.0

target_include_directories(vulkanite-config
  INTERFACE
    /home/wald/Projects/owl/owl/include
  )
vkn_embed_devicecode(
  OUTPUT_TARGET s02_embedded_spirv
  SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/deviceCode.slang
  SPIRV_LINK_LIBRARIES vulkanite-samples-config
)

add_executable(s02-twoGeometries hostCode.cpp)
target_link_libraries(s02-twoGeometries
  PRIVATE
  s02_embedded_spirv
  vulkanite::vulkanite
  vulkanite-samples-config
  )
add_test(s02-twoGeometries ${CMAKE_BINARY_DIR}/s02-twoGeometries)
