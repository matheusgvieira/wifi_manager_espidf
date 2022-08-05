# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/Users/mgois/esp/esp-idf/components/bootloader/subproject"
  "/Users/mgois/tcc/webserver_setup/build/bootloader"
  "/Users/mgois/tcc/webserver_setup/build/bootloader-prefix"
  "/Users/mgois/tcc/webserver_setup/build/bootloader-prefix/tmp"
  "/Users/mgois/tcc/webserver_setup/build/bootloader-prefix/src/bootloader-stamp"
  "/Users/mgois/tcc/webserver_setup/build/bootloader-prefix/src"
  "/Users/mgois/tcc/webserver_setup/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/mgois/tcc/webserver_setup/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
