cmake_minimum_required(VERSION 2.8.5 FATAL_ERROR)

if(POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW) # remove if CMake >= 2.8.11 required
endif()

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

if(vtkTestingCore_LOADED)
 add_executable(MultiBlock MACOSX_BUNDLE MultiBlock.cxx)
 target_link_libraries(MultiBlock ${VTK_LIBRARIES})
endif()
