find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

vtk_module_third_party(xdmf3
  LIBRARIES vtkxdmf3
  INCLUDE_DIRS
   ${CMAKE_CURRENT_SOURCE_DIR}/vtkxdmf3
   ${CMAKE_CURRENT_SOURCE_DIR}/vtkxdmf3/core
   ${CMAKE_CURRENT_BINARY_DIR}/vtkxdmf3
   ${CMAKE_CURRENT_BINARY_DIR}/vtkxdmf3/core
   ${CMAKE_CURRENT_SOURCE_DIR}/vtkxdmf3/CMake/VersionSuite
  NO_ADD_SUBDIRECTORY
)

if(VTK_USE_SYSTEM_XDMF3)
  return()
endif()

#########################################################################
# Set Xdmf build environment options the way VTK wants them

#?

#########################################################################
# Setup cmake to pull the library into place as a vtk module

vtk_module_export_info()
add_subdirectory(vtkxdmf3)

vtk_target_export(vtkxdmf3)
vtk_target_export(XdmfCore)
