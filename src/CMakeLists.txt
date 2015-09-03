cmake_minimum_required(VERSION 3.0)
project(SegyVisualizer2D)

FIND_PACKAGE(VTK REQUIRED)
INCLUDE(${VTK_USE_FILE})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp)
add_executable(SegyVisualizer2D ${SOURCE_FILES} SegyReader.cpp SegyReader.h BinaryHeaderBytesPositions.h TraceHeaderBytesPositions.h IOUtil.cpp IOUtil.h Trace.cpp Trace.h SegyTraceReader.cpp SegyTraceReader.h vtkSeismicSliceWidget.cpp vtkSeismicSliceWidget.h vtkSeismicSliceCallback.cpp vtkSeismicSliceCallback.h RdvReader.cpp RdvReader.h vtkSegy2DReader.cpp vtkSegy2DReader.h vtkSegy3DReader.cpp vtkSegy3DReader.h)
TARGET_LINK_LIBRARIES(SegyVisualizer2D ${VTK_LIBRARIES})