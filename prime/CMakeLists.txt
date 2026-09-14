# SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
# SPDX-License-Identifier: Apache-2.0

configure_file(
  ${PROJECT_SOURCE_DIR}/prime/include/vulkanite/prime.h
  include/vulkanite/prime.h
)
#target_include_directories(vulkanite-config
#  INTERFACE
#  ${CMAKE_CURRENT_LIST_DIR}/include>
#    /home/wald/Projects/owl/owl/include
#  )
vkn_embed_devicecode(
  OUTPUT_TARGET vkprime_embedded_spirv
  SOURCES ${CMAKE_CURRENT_LIST_DIR}/deviceCode.slang
  SPIRV_LINK_LIBRARIES vulkanite-config
#  INCLUDES ${VULKANITE_ROOT_DIR}/3rdParty/glm
)


add_library(vulkanite_prime
  Context.cpp
  Mesh.cpp
  Group.cpp
  Model.cpp
  prime.cpp
)
target_include_directories(vulkanite_prime PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include/>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(vulkanite_prime PUBLIC
  $<BUILD_INTERFACE:vkprime_embedded_spirv>
  $<BUILD_INTERFACE:glm::glm>
  vulkanite
#  vkprime_embedded_spirv
)

add_library(vulkanite::vulkanite_prime ALIAS vulkanite_prime)
install(TARGETS vulkanite_prime 
  EXPORT vulkanite-config
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  NAMELINK_SKIP
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  NAMELINK_SKIP
)




