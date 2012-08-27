cmake_minimum_required(VERSION 2.8.5 FATAL_ERROR)

PROJECT (MultiBlock)

if(NOT VTK_BINARY_DIR)
  find_package(VTK COMPONENTS
    vtkCommonCore
    vtkCommonDataModel
    vtkCommonExecutionModel
    vtkFiltersCore
    vtkFiltersGeneral
    vtkFiltersGeometry
    vtkFiltersSources
    vtkIOXML
    vtkRenderingCore
    vtkRenderingOpenGL
    vtkTestingCore
  )
include(${VTK_USE_FILE})
endif()

add_executable(MultiBlock MACOSX_BUNDLE MultiBlock.cxx)
target_link_libraries(MultiBlock ${VTK_LIBRARIES})
