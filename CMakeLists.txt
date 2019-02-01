cmake_minimum_required(VERSION 2.6)

project(ff_test)

set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

set(CMAKE_C_FLAGS "-g -mpreferred-stack-boundary=4 -mstackrealign")

set(CMAKE_CXX_FLAGS "-g -O0 -mpreferred-stack-boundary=4 -mstackrealign")

set(FF_INSTALL_PATH /mnt/sda7/project/ffmpeg/b32)
set(FF_SRC_PATH /mnt/sda7/project/ffmpeg/src_b32)

add_subdirectory(libsrc)
add_subdirectory(src)

