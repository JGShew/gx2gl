cmake_minimum_required(VERSION 3.2)
project(lesson02 CXX)
include("${DEVKITPRO}/wut/share/wut.cmake" REQUIRED)

add_executable(lesson02 lesson2.cpp)

wut_enable_stdcpp(lesson02)

target_link_libraries(lesson02
   m
   whb
   gfd
   gx2
   proc_ui
   nsysnet
   sysapp
   gx2gl)

target_include_directories(lesson02 PRIVATE "${WUT_ROOT}/include" "${CMAKE_SOURCE_DIR}/include")

wut_create_rpx(lesson02.rpx lesson02)
