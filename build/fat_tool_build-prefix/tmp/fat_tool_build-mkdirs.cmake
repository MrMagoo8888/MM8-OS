# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/home/mm8/Documents/OS/MM8/MM8-OS/tools/fat")
  file(MAKE_DIRECTORY "/home/mm8/Documents/OS/MM8/MM8-OS/tools/fat")
endif()
file(MAKE_DIRECTORY
  "/home/mm8/Documents/OS/MM8/MM8-OS/build/fat_tool-build"
  "/home/mm8/Documents/OS/MM8/MM8-OS/build/fat_tool-install"
  "/home/mm8/Documents/OS/MM8/MM8-OS/build/fat_tool_build-prefix/tmp"
  "/home/mm8/Documents/OS/MM8/MM8-OS/build/fat_tool_build-prefix/src/fat_tool_build-stamp"
  "/home/mm8/Documents/OS/MM8/MM8-OS/build/fat_tool_build-prefix/src"
  "/home/mm8/Documents/OS/MM8/MM8-OS/build/fat_tool_build-prefix/src/fat_tool_build-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/mm8/Documents/OS/MM8/MM8-OS/build/fat_tool_build-prefix/src/fat_tool_build-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/mm8/Documents/OS/MM8/MM8-OS/build/fat_tool_build-prefix/src/fat_tool_build-stamp${cfgdir}") # cfgdir has leading slash
endif()
