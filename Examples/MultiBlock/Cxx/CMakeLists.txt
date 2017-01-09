cmake_minimum_required(VERSION 3.3 FATAL_ERROR)


PROJECT (MultiBlock)

find_package(VTK REQUIRED)
vtk_module_config(VTK
  vtkCommonCore
  vtkFiltersGeometry
  vtkFiltersSources
  vtkIOXML
  vtkInteractionStyle
  vtkRendering${VTK_RENDERING_BACKEND}
  vtkTestingCore
  vtksys
)
include(${VTK_USE_FILE})

if(vtkTestingCore_LOADED)
 add_executable(MultiBlock MACOSX_BUNDLE MultiBlock.cxx)
 target_link_libraries(MultiBlock ${VTK_LIBRARIES})
endif()
