cmake_minimum_required(VERSION 2.8)

project(HyperTreeGridExamples)

if(NOT VTK_BINARY_DIR)
  find_package(VTK REQUIRED)
  include(${VTK_USE_FILE})
endif(NOT VTK_BINARY_DIR)

link_directories(${VTK_LIBRARY_DIRS})

# Add example showing use use of synthetic vtkHyperTreeGrid along with various filters
add_executable(FractalHyperTreeGridDemonstrator FractalHyperTreeGridDemonstrator.cxx)
target_link_libraries(FractalHyperTreeGridDemonstrator vtkCommonCore vtkCommonSystem vtkCommonDataModel vtkCommonExecutionModel vtkIOLegacy vtkIOGeometry vtkFiltersCore vtkFiltersSources vtkFiltersHyperTree)

# Add example showing use use of specified vtkHyperTreeGrid along with various filters
add_executable(DescribedHyperTreeGridDemonstrator DescribedHyperTreeGridDemonstrator.cxx)
target_link_libraries(DescribedHyperTreeGridDemonstrator vtkCommonCore vtkCommonSystem vtkCommonDataModel vtkCommonExecutionModel vtkIOLegacy vtkIOGeometry vtkFiltersCore vtkFiltersSources vtkFiltersHyperTree)