# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/root/esp-idf-latest/components/bootloader/subproject"
  "/root/ota-portal/build/bootloader"
  "/root/ota-portal/build/bootloader-prefix"
  "/root/ota-portal/build/bootloader-prefix/tmp"
  "/root/ota-portal/build/bootloader-prefix/src/bootloader-stamp"
  "/root/ota-portal/build/bootloader-prefix/src"
  "/root/ota-portal/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/root/ota-portal/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/root/ota-portal/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
