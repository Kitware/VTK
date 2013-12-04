cmake_minimum_required(VERSION 2.8.5 FATAL_ERROR)

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
  foreach(name ${HYBRID_EXAMPLES_SRCS})
    add_executable(${name} MACOSX_BUNDLE ${name}.cxx)
    target_link_libraries(${name} ${VTK_LIBRARIES})
  endforeach()
endif()
