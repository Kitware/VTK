cmake_minimum_required(VERSION 2.8.8)
if(POLICY CMP0025)
  cmake_policy(SET CMP0025 NEW) # CMake 3.0
endif()
if(POLICY CMP0053)
  cmake_policy(SET CMP0053 NEW) # CMake 3.1
endif()

project(HyperTreeGridExamples)

if(NOT VTK_BINARY_DIR)
  find_package(VTK REQUIRED)
  include(${VTK_USE_FILE})
endif()

link_directories(${VTK_LIBRARY_DIRS})

# Add example showing use use of synthetic vtkHyperTreeGrid along with various filters
add_executable(HyperTreeGridDemonstrator HyperTreeGridDemonstrator.cxx)
target_link_libraries(HyperTreeGridDemonstrator vtkCommonCore vtkCommonSystem vtkCommonDataModel vtkCommonExecutionModel vtkIOLegacy vtkIOGeometry vtkFiltersCore vtkFiltersSources vtkFiltersHyperTree)
