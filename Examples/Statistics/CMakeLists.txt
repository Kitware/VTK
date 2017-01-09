cmake_minimum_required(VERSION 3.3 FATAL_ERROR)


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
