cmake_minimum_required(VERSION 2.8.4)
project(otto-sdk)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

if(UNIX)
    # Both APPLE and UNIX are defined on OSX
    if(APPLE)
        set(STAK_TARGET_APPLE 1)
        set(STAK_DEVELOPMENT_MODE 1)
        set(STAK_DEBUG_ENABLE 1)
    endif()

    # NaCl and Emscripten are Unix too, this selects only the other ones
    set(STAK_TARGET_UNIX 1)
endif()

# include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

# add_subdirectory(src)

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CORRADE_CXX_FLAGS}")
# include_directories(${MAGNUM_INCLUDE_DIRS} ${MAGNUM_APPLICATION_INCLUDE_DIRS})

include_directories( src lib)
add_executable( main
                src/main.cpp
                src/application/application.cpp)
# target_link_libraries()

# include_directories(/opt/vc/include)
# include_directories(/opt/vc/userland-master/interface/vcos)
# include_directories(/opt/vc/userland-master)
# include_directories(/opt/vc/userland-master/interface/vcos/pthreads)
# include_directories(/opt/vc/userland-master/interface/vmcs_host/linux)
# include_directories(/opt/vc/userland-master/host_applications/linux/libs/bcm_host/include)
# link_directories(/opt/vc/lib)
# add_executable(picamdemo picam.cpp camera.cpp cameracontrol.cpp graphics.cpp)

# target_link_libraries(picamdemo libmmal_core.so libmmal_util.so libmmal_vc_client.so libvcos.so libbcm_host.so GLESv2 EGL ${OpenCV_LIBS})


# set(SOURCE_FILES main.c)

# add_executable(otto_sdk ${SOURCE_FILES})