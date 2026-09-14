# SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
# SPDX-License-Identifier: Apache-2.0

vkn_embed_devicecode(
  OUTPUT_TARGET s01_embedded_spirv
  SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/deviceCode.slang
  SPIRV_LINK_LIBRARIES vulkanite-samples-config 
)

add_executable(s01-simpleTriangles hostCode.cpp)
target_link_libraries(s01-simpleTriangles
  PRIVATE
  s01_embedded_spirv
  vulkanite::vulkanite
  vulkanite-samples-config
)
add_test(s01-simpleTriangles ${CMAKE_BINARY_DIR}/s01-simpleTriangles)
