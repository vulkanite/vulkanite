# SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
# SPDX-License-Identifier: Apache-2.0

# vulkanite/apps/ : defines various apps/tools/test cases for vulkanite

# ==================================================================
# vknPrintSanitize - helper tool to make vulkan debug layer printf()
# messages more readable. To use: run your program as usual, but pipe
# the output through vknPrintSanitize; the output should looks more
# more sane. Note that device side prints will ONLY appear if the
# VulkanSDK is installed and loaded in the first place!
# ==================================================================
add_executable(vknPrintSanitize printSanitize.cpp)
target_link_libraries(vknPrintSanitize vulkanite-config)

install(TARGETS vknPrintSanitize
  EXPORT vulkanite-config
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  NAMELINK_SKIP
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  NAMELINK_SKIP
)


