# SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
# SPDX-License-Identifier: Apache-2.0

file(GLOB samples RELATIVE ${CMAKE_CURRENT_LIST_DIR} "s??-*")
#set(samples s01-simpleTriangles)

add_library(vulkanite-samples-config INTERFACE)
target_link_libraries(vulkanite-samples-config INTERFACE
  vulkanite-config )
target_include_directories(vulkanite-samples-config INTERFACE
  ${PROJECT_SOURCE_DIR}/3rdParty/glm
  ${PROJECT_SOURCE_DIR}/3rdParty/stb_image
)
foreach(sample ${samples})
  add_subdirectory(${sample})
endforeach()
