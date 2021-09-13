cmake_minimum_required(VERSION 2.8)
project(video_stream_server)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11")

if (CMAKE_HOST_WIN32)
    set(WINDOWS 1)
elseif (CMAKE_HOST_APPLE)
    set(MACOS 1)
elseif (CMAKE_HOST_UNIX)
    set(LINUX 1)
endif ()

if (MACOS)
    #    set(Boost_ROOT /usr/local/opt/boost@1.59)
    set(Boost_INCLUDE_DIR /usr/local/Cellar/boost@1.59/1.59.0/include)  # add My MacOS libboost

    find_package(Boost 1.59.0 COMPONENTS system filesystem REQUIRED)
    find_package(PkgConfig REQUIRED)
elseif (LINUX)
    find_package(Boost 1.53.0 REQUIRED COMPONENTS system filesystem)
    find_package(PkgConfig REQUIRED)
endif ()


if (MACOS)
    include_directories(
            ${PROJECT_SOURCE_DIR}/include
            ${Boost_INCLUDE_DIRS}
            /usr/local/include
    )
    link_directories(
            ${PROJECT_SOURCE_DIR}/lib
            /usr/local/lib
    )
#    link_libraries(/Users/nvidia/CLionProjects/video_stream/lib/libjson_linux-gcc-11.0.3_libmt.dylib)
elseif (LINUX)
    include_directories(/usr/local/include
            /home/nvidia/ffmpeg_build/include
            ${PROJECT_SOURCE_DIR}/include
            )

    link_directories(/usr/local/lib
            /usr/lib64
            ${PROJECT_SOURCE_DIR}/lib
            /home/nvidia/ffmpeg_build/lib)
endif ()

add_executable(video_stream_server src/main.cpp src/VideoStreamServer.cpp src/VideoStreamServer.h)

#install(TARGETS video_stream_server RUNTIME DESTINATION bin)
if (MACOS)
    target_link_libraries(video_stream_server pthread ${Boost_LIBRARIES} -ljson_linux-gcc-11.0.3_libmt -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lswresample -lswscale)
elseif (LINUX)
    target_link_libraries(video_stream_server X11 pthread libjson_linux-gcc-4.8.5_libmt.so libavcodec.so libavdevice.so libavfilter.so libavformat.so libavutil.so libswresample.so libswscale.so ${Boost_LIBRARIES})
endif ()

#-rpath=/Users/nvidia/CLionProjects/video_stream/lib/libjson_linux-gcc-11.0.3_libmt.dylib
