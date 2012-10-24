cmake_minimum_required(VERSION 2.8)

project(HyperTreeGridExamples)

if(NOT VTK_BINARY_DIR)
  find_package(VTK REQUIRED)
  include(${VTK_USE_FILE})
endif(NOT VTK_BINARY_DIR)

link_directories(${VTK_LIBRARY_DIRS})

# Add 2D example showing use use of synthetic vtkHyperTreeGrid along with various filters
add_executable(HyperTreeGridDemonstrator2D HyperTreeGridDemonstrator2D.cxx)
target_link_libraries(HyperTreeGridDemonstrator2D vtkCommonCore vtkCommonSystem vtkCommonDataModel vtkCommonExecutionModel vtkIOLegacy vtkIOGeometry vtkFiltersCore vtkFiltersSources vtkFiltersHyperTree)

# Add 3D example showing use use of synthetic vtkHyperTreeGrid along with various filters
add_executable(HyperTreeGridDemonstrator3D HyperTreeGridDemonstrator3D.cxx)
target_link_libraries(HyperTreeGridDemonstrator3D vtkCommonCore vtkCommonSystem vtkCommonDataModel vtkCommonExecutionModel vtkIOLegacy vtkIOGeometry vtkFiltersCore vtkFiltersSources vtkFiltersHyperTree)
