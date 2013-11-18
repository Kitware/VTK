cmake_minimum_required(VERSION 2.8.5 FATAL_ERROR)

PROJECT (MultiBlock)

find_package(VTK COMPONENTS
  vtkCommonCore
  vtkFiltersGeometry
  vtkFiltersSources
  vtkIOXML
  vtkInteractionStyle
  vtkRenderingOpenGL
  vtkTestingCore
  vtksys
)
include(${VTK_USE_FILE})

if(vtkTestingCore_LOADED)
 add_executable(MultiBlock MACOSX_BUNDLE MultiBlock.cxx)
 target_link_libraries(MultiBlock ${VTK_LIBRARIES})
endif()
