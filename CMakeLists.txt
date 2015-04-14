cmake_minimum_required(VERSION 2.8)
project(otto-runner)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y")

include_directories(
  src
  lib
  include
)
include_directories(SYSTEM
  # VideoCore
  "${VC_SDK}/include"
  "${VC_SDK}/include/interface/vcos/pthreads"
  "${VC_SDK}/include/interface/vmcs_host/linux"
)
message("VC_SDK=${VC_SDK}")
link_directories(
  # VideoCore
  "${VC_SDK}/lib" 

  # Otto SDK Main
  "${CMAKE_CURRENT_SOURCE_DIR}../lib"
)

set(src
  "lib/graphics/seps114a/seps114a.c"
  "lib/graphics/canvas/canvas.c"
  "lib/graphics/canvas/canvas.c"
  "src/application/application.c"
  "src/main.c"
)

set(mode_src ${src})
add_executable( otto-runner
  ${src}
)

target_include_directories (otto-runner PUBLIC src)
target_link_libraries( otto-runner
  pthread
  dl
  stdc++
  c
  m
  bcm_host
  vchostif
  GLESv2
  EGL
  rt
  OttoHardware
  bcm2835
)
