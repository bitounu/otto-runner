cmake_minimum_required(VERSION 2.8)
project(otto-runner)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y")

set(VC_SDK    "/opt/vc")
set(STAK_SDK  "/opt/stak/sdk")
set(OTTO_BUILD_ROOT  "${STAK_SDK}/../otto")

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

  # Stak
  "${STAK_SDK}/include"
)
link_directories(
  # VideoCore
  "${VC_SDK}/lib"

  # Stak
  "${STAK_SDK}/lib"

  # Otto SDK Main
  "${CMAKE_CURRENT_SOURCE_DIR}../lib"
  # "${STAK_SDK}/../otto/libOttoHardware/include"
)

set(src
  "lib/graphics/seps114a/seps114a.c"
  "lib/graphics/canvas/canvas.c"
  "lib/graphics/canvas/canvas.c"
  "src/application/application.c"
  "src/main.c"
)

set(mode_src ${src})
# add_subdirectory( ../libraries/libOttoHardware ${CMAKE_CURRENT_BINARY_DIR}/libOttoHardware )
add_executable( otto-runner
  ${src}
)
# find_package( libOttoHardware )
# add_custom_target( OttoHardware)
# add_dependencies( otto-runner OttoHardware )
target_include_directories (otto-runner PUBLIC src)
target_link_libraries( otto-runner
  pthread
  dl
  stdc++
  c
  m
  bcm_host
  GLESv2
  EGL
  rt
  OttoHardware
  bcm2835
)
