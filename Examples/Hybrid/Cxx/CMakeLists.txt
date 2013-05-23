cmake_minimum_required(VERSION 2.8.5 FATAL_ERROR)

PROJECT (Hybrid)

if(NOT VTK_SOURCE_DIR)
  message(ERROR "Cannot build Hybrid examples without VTK_SOURCE_DIR")
endif()

if(NOT VTK_BINARY_DIR)
  find_package(VTK REQUIRED)
  include(${VTK_USE_FILE})
endif()

include_directories(
  # ImageDataLIC2DDemo and StructuredGridLIC2DDemo include
  # TestImageDataLIC2D.h  and TestStructuredGridLIC2DSlice
  # from the dir below
  ${VTK_SOURCE_DIR}/Rendering/HybridOpenGL/Testing/Cxx
)

set(HYBRID_EXAMPLES_SRCS
  ImageDataLIC2DDemo
  StructuredGridLIC2DDemo
  )

if(vtkTestingCore_LOADED)
  foreach(name ${HYBRID_EXAMPLES_SRCS})
    add_executable(${name} MACOSX_BUNDLE ${name}.cxx)
    target_link_libraries(${name} ${VTK_LIBRARIES} )
  endforeach()
endif()
