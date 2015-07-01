cmake_minimum_required(VERSION 3.0)
project(SegyVisualizer)

FIND_PACKAGE(VTK REQUIRED)
INCLUDE(${VTK_USE_FILE})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp)
add_executable(SegyVisualizer ${SOURCE_FILES} SegyReader.cpp SegyReader.h BinaryHeaderBytesPositions.h TraceHeaderBytesPositions.h)
TARGET_LINK_LIBRARIES(SegyVisualizer ${VTK_LIBRARIES})