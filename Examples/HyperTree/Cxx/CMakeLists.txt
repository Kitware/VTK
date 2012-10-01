cmake_minimum_required(VERSION 2.8)

project(HyperTreeGridDemonstrator)

if(NOT VTK_BINARY_DIR)
  find_package(VTK REQUIRED)
  include(${VTK_USE_FILE})
endif(NOT VTK_BINARY_DIR)

link_directories(${VTK_LIBRARY_DIRS})

# Add example showing use use of vtkHyperTreeGrid along with various filters
add_executable(HyperTreeGridDemonstrator HyperTreeGridDemonstrator.cxx)
target_link_libraries(HyperTreeGridDemonstrator vtkCommonCore vtkCommonSystem vtkCommonDataModel vtkCommonExecutionModel vtkIOGeometry vtkFiltersCore vtkFiltersSources vtkFiltersHyperTree)
