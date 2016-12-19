cmake_minimum_required (VERSION 2.8.8)


project(HyperTreeGridSphere)

if(NOT VTK_BINARY_DIR)
  find_package(VTK REQUIRED)
  include(${VTK_USE_FILE})
endif()

link_directories(${VTK_LIBRARY_DIRS})

set(HyperTreeGridSphere_SRCS
  Cell.cxx
  Node.cxx
  Mesh.cxx
  HyperTreeGridSphere.cxx)

# Add example showing use use of synthetic vtkHyperTreeGrid along with various filters
add_executable(HyperTreeGridSphere ${HyperTreeGridSphere_SRCS})
target_link_libraries(HyperTreeGridSphere vtkCommonCore vtkCommonSystem vtkCommonDataModel vtkCommonExecutionModel vtkIOCore vtkIOLegacy vtkFiltersCore vtkFiltersGeometry vtkFiltersHyperTree vtkInteractionStyle vtkRenderingCore vtkRenderingOpenGL)
