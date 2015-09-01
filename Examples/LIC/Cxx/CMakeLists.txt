cmake_minimum_required(VERSION 2.8.5 FATAL_ERROR)
if(POLICY CMP0025)
  cmake_policy(SET CMP0025 NEW) # CMake 3.0
endif()
if(POLICY CMP0053)
  cmake_policy(SET CMP0053 NEW) # CMake 3.1
endif()

PROJECT(LIC)

if(NOT VTK_SOURCE_DIR)
  message(ERROR " Cannot build LIC examples without VTK_SOURCE_DIR")
endif()

find_package(VTK COMPONENTS
  vtkFiltersExtraction
  vtkFiltersGeometry
  vtkIOImage
  vtkInteractionStyle
  vtkRenderingLIC
  vtkTestingRendering
  vtksys
)
include(${VTK_USE_FILE})

# these demos are simply repackaged ctests
# by #include'ing the test implementation
include_directories(
  ${VTK_SOURCE_DIR}/Rendering/LIC/Testing/Cxx
  )

set(HYBRID_EXAMPLES_SRCS
  ImageDataLIC2DDemo
  StructuredGridLIC2DDemo
  SurfaceLICDemo
  )

if(vtkTestingCore_LOADED)
  include(vtkOpenGL)
  foreach(name ${HYBRID_EXAMPLES_SRCS})
    add_executable(${name} MACOSX_BUNDLE ${name}.cxx)
    target_link_libraries(${name} LINK_PRIVATE ${VTK_LIBRARIES})
    vtk_opengl_link(${name})
  endforeach()
endif()
