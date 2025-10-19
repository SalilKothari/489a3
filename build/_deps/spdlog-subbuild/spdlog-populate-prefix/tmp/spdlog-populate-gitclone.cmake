# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

if(EXISTS "/root/eecs489/p3/489a3/build/_deps/spdlog-subbuild/spdlog-populate-prefix/src/spdlog-populate-stamp/spdlog-populate-gitclone-lastrun.txt" AND EXISTS "/root/eecs489/p3/489a3/build/_deps/spdlog-subbuild/spdlog-populate-prefix/src/spdlog-populate-stamp/spdlog-populate-gitinfo.txt" AND
  "/root/eecs489/p3/489a3/build/_deps/spdlog-subbuild/spdlog-populate-prefix/src/spdlog-populate-stamp/spdlog-populate-gitclone-lastrun.txt" IS_NEWER_THAN "/root/eecs489/p3/489a3/build/_deps/spdlog-subbuild/spdlog-populate-prefix/src/spdlog-populate-stamp/spdlog-populate-gitinfo.txt")
  message(STATUS
    "Avoiding repeated git clone, stamp file is up to date: "
    "'/root/eecs489/p3/489a3/build/_deps/spdlog-subbuild/spdlog-populate-prefix/src/spdlog-populate-stamp/spdlog-populate-gitclone-lastrun.txt'"
  )
  return()
endif()

execute_process(
  COMMAND ${CMAKE_COMMAND} -E rm -rf "/root/eecs489/p3/489a3/build/_deps/spdlog-src"
  RESULT_VARIABLE error_code
)
if(error_code)
  message(FATAL_ERROR "Failed to remove directory: '/root/eecs489/p3/489a3/build/_deps/spdlog-src'")
endif()

# try the clone 3 times in case there is an odd git clone issue
set(error_code 1)
set(number_of_tries 0)
while(error_code AND number_of_tries LESS 3)
  execute_process(
    COMMAND "/usr/bin/git"
            clone --no-checkout --config "advice.detachedHead=false" "https://github.com/gabime/spdlog.git" "spdlog-src"
    WORKING_DIRECTORY "/root/eecs489/p3/489a3/build/_deps"
    RESULT_VARIABLE error_code
  )
  math(EXPR number_of_tries "${number_of_tries} + 1")
endwhile()
if(number_of_tries GREATER 1)
  message(STATUS "Had to git clone more than once: ${number_of_tries} times.")
endif()
if(error_code)
  message(FATAL_ERROR "Failed to clone repository: 'https://github.com/gabime/spdlog.git'")
endif()

execute_process(
  COMMAND "/usr/bin/git"
          checkout "v1.14.1" --
  WORKING_DIRECTORY "/root/eecs489/p3/489a3/build/_deps/spdlog-src"
  RESULT_VARIABLE error_code
)
if(error_code)
  message(FATAL_ERROR "Failed to checkout tag: 'v1.14.1'")
endif()

set(init_submodules TRUE)
if(init_submodules)
  execute_process(
    COMMAND "/usr/bin/git" 
            submodule update --recursive --init 
    WORKING_DIRECTORY "/root/eecs489/p3/489a3/build/_deps/spdlog-src"
    RESULT_VARIABLE error_code
  )
endif()
if(error_code)
  message(FATAL_ERROR "Failed to update submodules in: '/root/eecs489/p3/489a3/build/_deps/spdlog-src'")
endif()

# Complete success, update the script-last-run stamp file:
#
execute_process(
  COMMAND ${CMAKE_COMMAND} -E copy "/root/eecs489/p3/489a3/build/_deps/spdlog-subbuild/spdlog-populate-prefix/src/spdlog-populate-stamp/spdlog-populate-gitinfo.txt" "/root/eecs489/p3/489a3/build/_deps/spdlog-subbuild/spdlog-populate-prefix/src/spdlog-populate-stamp/spdlog-populate-gitclone-lastrun.txt"
  RESULT_VARIABLE error_code
)
if(error_code)
  message(FATAL_ERROR "Failed to copy script-last-run stamp file: '/root/eecs489/p3/489a3/build/_deps/spdlog-subbuild/spdlog-populate-prefix/src/spdlog-populate-stamp/spdlog-populate-gitclone-lastrun.txt'")
endif()
