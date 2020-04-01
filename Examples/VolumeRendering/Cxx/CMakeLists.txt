cmake_minimum_required(VERSION 3.8...3.12 FATAL_ERROR)


project (VolumeRendering)
find_package(VTK REQUIRED)
vtk_module_config(VTK
  vtkCommonCore
  vtkIOImage
  vtkIOXML
  vtkInteractionWidgets
  vtkRenderingVolumeOpenGL2
)
include(${VTK_USE_FILE})

set(RENDERING_EXAMPLES_SRCS
  GPURenderDemo
  FixedPointVolumeRayCastMapperCT
)

foreach(name ${RENDERING_EXAMPLES_SRCS})
  add_executable(${name} MACOSX_BUNDLE ${name}.cxx)
  target_link_libraries(${name} ${VTK_LIBRARIES})
endforeach()



