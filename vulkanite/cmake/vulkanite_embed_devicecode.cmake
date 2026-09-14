# SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
# SPDX-License-Identifier: Apache-2.0

# based on an earlier (and much simpler) version from nate morrical's
# GPRT version, under this license:

# MIT License

# Copyright (c) 2022 Nathan V. Morrical

cmake_minimum_required(VERSION 4.0)

include(FetchContent)

find_program(CMAKE_SLANG_COMPILER slangc)

#if (CMAKE_SLANG_COMPILER)
  message("Found slangc: ${CMAKE_SLANG_COMPILER}")
# else ()
#   message("Slang compiler not defined...")
#   message("Downloading Slang Compiler...")
#   if (WIN32)
#     FetchContent_Declare(SlangCompiler
#       URL https://github.com/shader-slang/slang/releases/download/v2024.1.21/slang-2024.1.21-win64.zip
#       URL_HASH SHA256=CA1C8E6E85C2BC5B9180A2160DDF2F348201CE581AE6826D70D9B87CDF361E83
#       DOWNLOAD_NO_EXTRACT true
#       DOWNLOAD_DIR "${CMAKE_BINARY_DIR}/"
#     )
#     FetchContent_Populate(SlangCompiler)
#     message("Extracting...")
#     execute_process(COMMAND powershell Expand-Archive -Force -Path "${CMAKE_BINARY_DIR}/slang-2024.1.21-win64.zip" -DestinationPath "${CMAKE_BINARY_DIR}/slang-2024.1.21-win64" RESULT_VARIABLE EXTRACT_RESULT)
#     if(NOT EXTRACT_RESULT EQUAL 0)
#       message(FATAL_ERROR "Extraction failed with error code: ${EXTRACT_RESULT}")
#     else()
#       message("Done.")
#     endif()
#     set(CMAKE_SLANG_COMPILER "${CMAKE_BINARY_DIR}/slang-2024.1.21-win64/bin/windows-x64/release/slangc.exe" CACHE INTERNAL "CMAKE_SLANG_COMPILER")
#   else() # linux
#     FetchContent_Declare(HLSLCompiler
#       URL https://github.com/shader-slang/slang/releases/download/v2024.1.21/slang-2024.1.21-linux-x86_64.tar.gz
#       URL_HASH SHA256=309d18f00ce8c74ff2db438e68c0a4620a59cec0ae32ea40f9765adc1764e55e
#       DOWNLOAD_NO_EXTRACT true
#       DOWNLOAD_DIR "${CMAKE_BINARY_DIR}/"
#     )
#     FetchContent_Populate(HLSLCompiler)
#     message("Extracting...")
#     execute_process(COMMAND mkdir -p "${CMAKE_BINARY_DIR}/slang-2024.1.21-linux-x86_64" RESULT_VARIABLE EXTRACT_RESULT)
#     if(NOT EXTRACT_RESULT EQUAL 0)
#       message(FATAL_ERROR "mkdir failed with error code: ${EXTRACT_RESULT}")
#     endif()
#     execute_process(COMMAND tar xzf "${CMAKE_BINARY_DIR}/slang-2024.1.21-linux-x86_64.tar.gz" -C "${CMAKE_BINARY_DIR}/slang-2024.1.21-linux-x86_64" RESULT_VARIABLE EXTRACT_RESULT)
#     if(NOT EXTRACT_RESULT EQUAL 0)
#       message(FATAL_ERROR "tar failed with error code: ${EXTRACT_RESULT}")
#     else()
#       message("Done.")
#     endif()
#     execute_process(COMMAND chmod +x "${CMAKE_BINARY_DIR}/slang-2024.1.21-linux-x86_64/bin/linux-x64/release/slangc")
#     set(CMAKE_SLANG_COMPILER "${CMAKE_BINARY_DIR}/slang-2024.1.21-linux-x86_64/bin/linux-x64/release/slangc" CACHE INTERNAL "CMAKE_SLANG_COMPILER")
#   endif()

#   # Test to see if compiler is working
#   execute_process(COMMAND ${CMAKE_SLANG_COMPILER} "-v" RESULT_VARIABLE EXTRACT_RESULT)
#   if(NOT EXTRACT_RESULT EQUAL 0)
#     message("Envoking Slang compiler failed with error code: ${EXTRACT_RESULT}")
#   endif()
# endif()

set(EMBED_DEVICECODE_DIR ${CMAKE_CURRENT_LIST_DIR} CACHE INTERNAL "")

function(target_gather_includes)
	#  message("=======================================================")
  set(unresolved ${ARGN})
  #message("GATHERING INCLUDES FROM ${unresolved}")
  #message("=======================================================")
  set(gathered_includes "")
  while (unresolved)
	  #message(">>> current unresolved ${unresolved}")
    set(new_unresolved "")
    foreach (dep IN ITEMS ${unresolved})
	    #message(">> resolving ${dep}")
      if (dep MATCHES "<BUILD_INTERFACE:")
	      #message("dep matches build interface")
        string(LENGTH ${dep} len_dep)
        math(EXPR strip_len "${len_dep}-19")
        string(SUBSTRING "${dep}" 18 ${strip_len} stripped)
	#message(" -> stripped ${stripped}")
#        list(APPEND new_unresolved ${stripped})
        set(dep ${stripped})
      endif()
      if (EXISTS ${dep})
	      #message(" .... is a file - skipping")
        continue()
      endif()
      if (NOT (TARGET ${dep}))
	      #message(" .... is NOT a target - skipping")
        continue()
      endif()
      get_target_property(target_type ${dep} TYPE)
      if ((TARGET ${dep}) OR (${target_type} STREQUAL "INTERFACE_LIBRARY"))
        get_property(include_dirs TARGET "${dep}" PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
	#message(" -> include_dirs ${include_dirs}")
        foreach (item IN ITEMS ${include_dirs})
          if (item MATCHES "<INSTALL_INTERFACE:")
            continue()
          endif()
          if (item MATCHES "<BUILD_INTERFACE:")
		  #message("ite matches build interface")
            string(LENGTH ${item} len_item)
            math(EXPR strip_len "${len_item}-19")
            string(SUBSTRING "${item}" 18 ${strip_len} stripped)
	    #message(" -> stripped ${stripped}")
            #        list(APPEND new_unresolved ${stripped})
            set(item ${stripped})
          endif()
          list(APPEND gathered_includes ${item})
        endforeach()
        get_property(link_libs TARGET "${dep}" PROPERTY INTERFACE_LINK_LIBRARIES)
	#message(" -> link_libs ${link_libs}")
        foreach (tgt IN ITEMS ${link_libs})
          list(APPEND new_unresolved ${tgt})
        endforeach()
      else()
	      #message("  -->>> not a target ${tgt}")
      endif()
    endforeach()
    set(unresolved ${new_unresolved})
    #message("--> new unresolved ${unresolved}")
    #message("--> final gathered ${gathered_includes}")
    set(target_gathered_includes ${gathered_includes} PARENT_SCOPE)
  endwhile()

endfunction()

  
function(vkn_embed_devicecode)
  # processes arguments given to a function, and defines a set of variables
  #   which hold the values of the respective options
  set(oneArgs OUTPUT_TARGET)
  set(multiArgs SPIRV_LINK_LIBRARIES SOURCES HEADERS INCLUDES)
  cmake_parse_arguments(EMBED_DEVICECODE "" "${oneArgs}" "${multiArgs}" ${ARGN})

  unset(EMBED_DEVICECODE_OUTPUTS)

#  set (EMBED_INCLUDE_PATHS_STRING "")
#  set (EMBED_INCLUDE_PATHS_LIST "")
  set (JOIN_LIST "")
#  set (JOIN_STRING "-Ibla")
#  set(EMBED_INCLUDE_PATHS_STRING "${VULKANITE_ROOT_DIR}/vulkanite/include")
  # this next call will set a (list) variable "taget_gatheredd_includes":
  target_gather_includes(${EMBED_DEVICECODE_SPIRV_LINK_LIBRARIES})
  foreach(dir IN ITEMS ${target_gathered_includes})
#    list(APPEND EMBED_INCLUDE_PATHS_STRING "-I${dir}")
#    string(APPEND EMBED_INCLUDE_PATHS_STRING ";-I${dir}")
    list(APPEND JOIN_LIST "-I${dir}")
#    string(APPEND JOIN_STRING ";-I${dir}")
  endforeach()

  # message("custom target to make ${CMAKE_CURRENT_BINARY_DIR}/${EMBED_DEVICECODE_OUTPUT_TARGET}.spv")
  #message("from sources ${EMBED_DEVICECODE_SOURCES}")
#  message("cmd ${CMAKE_SLANG_COMPILER}    ${EMBED_DEVICECODE_SOURCES}    -profile sm_6_7    -target spirv -emit-spirv-directly    -force-glsl-scalar-layout    -fvk-use-entrypoint-name    -matrix-layout-row-major    -ignore-capabilities    -D__VKN_DEVICE__=1    ${EMBED_INCLUDE_PATHS_STRING} -o ${CMAKE_CURRENT_BINARY_DIR}/${EMBED_DEVICECODE_OUTPUT_TARGET}.spv")
#message("JOIN_LIST ${JOIN_LIST}")
  #message("join string ${JOIN_STRING}")
#  message("EMBED_INCLUDE_PATHS_STRING ${EMBED_INCLUDE_PATHS_STRING}")
  add_custom_command(
    # add_custom_target(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${EMBED_DEVICECODE_OUTPUT_TARGET}.spv
    COMMAND ${CMAKE_SLANG_COMPILER}
    ARGS
    ${JOIN_LIST}
    ${EMBED_DEVICECODE_SOURCES}
    -profile sm_6_7
    -target spirv -emit-spirv-directly
    -force-glsl-scalar-layout
    -fvk-use-entrypoint-name
    -matrix-layout-row-major
    -ignore-capabilities
    -D__VKN_DEVICE__=1
    -o ${CMAKE_CURRENT_BINARY_DIR}/${EMBED_DEVICECODE_OUTPUT_TARGET}.spv
#    CODEGEN
    COMMAND_EXPAND_LISTS
    DEPENDS ${EMBED_DEVICECODE_SOURCES} ${EMBED_DEVICECODE_HEADERS}
    IMPLICIT_DEPENDS CXX ${EMBED_DEVICECODE_SOURCES}
  )
  #    "${EMBED_INCLUDE_PATHS_STRING}"
    #    "-I$<JOIN:${JOIN_LIST},;-I>"
#    VERBATIM
    #${GPRT_INCLUDE_DIR}/gprt.slangh ${GPRT_INCLUDE_DIR}/gprt.h

  list(APPEND EMBED_DEVICECODE_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${EMBED_DEVICECODE_OUTPUT_TARGET}.spv)

  # Embed spirv as binary in a .cpp file
  set(EMBED_DEVICECODE_RUN ${EMBED_DEVICECODE_DIR}/vulkanite_bin2c.cmake)
  set(EMBED_DEVICECODE_CPP_FILE ${EMBED_DEVICECODE_OUTPUT_TARGET}.cpp)
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${EMBED_DEVICECODE_OUTPUT_TARGET}.cpp
    COMMAND ${CMAKE_COMMAND}
#    "-DINPUT_FILE=${EMBED_DEVICECODE_OUTPUT}"
    "-DINPUT_FILE=${CMAKE_CURRENT_BINARY_DIR}/${EMBED_DEVICECODE_OUTPUT_TARGET}.spv"
    "-DOUTPUT_C=${CMAKE_CURRENT_BINARY_DIR}/${EMBED_DEVICECODE_CPP_FILE}"
    "-DOUTPUT_VAR=${EMBED_DEVICECODE_OUTPUT_TARGET}"
    -P ${EMBED_DEVICECODE_RUN}
    VERBATIM
    COMMAND_EXPAND_LISTS
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${EMBED_DEVICECODE_OUTPUT_TARGET}.spv
#    DEPENDS ${EMBED_DEVICECODE_OUTPUT} ${CMAKE_CURRENT_BINARY_DIR}/${EMBED_DEVICECODE_OUTPUT_TARGET}.spv
  )

  message("creating object library '${EMBED_DEVICECODE_OUTPUT_TARGET}' with source '${EMBED_DEVICECODE_CPP_FILE}'")
  add_library(${EMBED_DEVICECODE_OUTPUT_TARGET}
    OBJECT)
  target_sources(${EMBED_DEVICECODE_OUTPUT_TARGET} PRIVATE
    ${EMBED_DEVICECODE_CPP_FILE}
  )
  set_target_properties(${EMBED_DEVICECODE_OUTPUT_TARGET}
    PROPERTIES
    POSITION_INDEPENDENT_CODE    ON
  )
  
endfunction()
