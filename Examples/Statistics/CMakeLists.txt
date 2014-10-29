cmake_minimum_required(VERSION 2.8.8)
if(POLICY CMP0025)
  cmake_policy(SET CMP0025 NEW) # CMake 3.0
endif()
if(POLICY CMP0053)
  cmake_policy(SET CMP0053 NEW) # CMake 3.1
endif()

project(VTKStatExample)

if(NOT VTK_BINARY_DIR)
  find_package(VTK REQUIRED)
  include(${VTK_USE_FILE})
endif()

link_directories(${VTK_LIBRARY_DIRS})

# Add example use of vtkDescriptiveStatistics
add_executable(ExampleDescriptiveStatistics ExampleDescriptiveStatistics.cxx)
target_link_libraries(ExampleDescriptiveStatistics vtkCommonCore vtkCommonSystem vtkCommonDataModel vtkFiltersStatistics)

# Add example use of vtkKMeansStatistics
add_executable(ExampleKMeansStatistics ExampleKMeansStatistics.cxx)
target_link_libraries(ExampleKMeansStatistics vtkCommonCore vtkCommonSystem vtkCommonDataModel vtkFiltersStatistics)
