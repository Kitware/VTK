cmake_minimum_required(VERSION 3.3 FATAL_ERROR)


project(HyperTreeGridExamples)

if(NOT VTK_BINARY_DIR)
  find_package(VTK REQUIRED)
  include(${VTK_USE_FILE})
endif()

link_directories(${VTK_LIBRARY_DIRS})

# Add example showing use use of synthetic vtkHyperTreeGrid along with various filters
add_executable(HyperTreeGridDemonstrator HyperTreeGridDemonstrator.cxx)
target_link_libraries(HyperTreeGridDemonstrator vtkCommonCore vtkCommonSystem vtkCommonDataModel vtkCommonExecutionModel vtkIOLegacy vtkIOGeometry vtkFiltersCore vtkFiltersSources vtkFiltersHyperTree)
