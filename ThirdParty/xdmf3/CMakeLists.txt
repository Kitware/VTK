find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
set(${vtk-module}_SYSTEM_INCLUDE_DIRS
    ${Boost_INCLUDE_DIRS})

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
set(REQUESTED_PYTHON_VERSION ${VTK_PYTHON_VERSION})
set(XDMF_BUILD_CORE_ONLY OFF CACHE INTERNAL "")
set(XDMF_BUILD_DOCUMENTATION OFF CACHE INTERNAL "")
set(XDMF_BUILD_DSM OFF CACHE INTERNAL "")
set(XDMF_BUILD_TESTING OFF CACHE INTERNAL "")
set(XDMF_BUILD_UTILS OFF CACHE INTERNAL "")
set(XDMF_LIBNAME "vtkxdmf3" CACHE INTERNAL "")
set(XDMF_NO_REALPATH ON CACHE INTERNAL "")
set(XDMF_WRAP_JAVA OFF CACHE INTERNAL "")
set(XDMF_WRAP_PYTHON OFF CACHE INTERNAL "")
set(HDF5_LIBRARIES ${vtkhdf5_LIBRARIES})
set(HDF5_hdf5_LIBRARY_RELEASE ${vtkhdf5_LIBRARIES})
set(HDF5_INCLUDE_DIRS ${vtkhdf5_INCLUDE_DIRS})
set(HDF5_C_INCLUDE_DIR ${vtkhdf5_INCLUDE_DIRS})
set(HDF5_FOUND TRUE)
set(LIBXML2_LIBRARIES ${vtklibxml2_LIBRARIES})
set(LIBXML2_INCLUDE_DIR ${vtklibxml2_INCLUDE_DIRS})
set(LIBXML2_FOUND TRUE)

#########################################################################
# Setup cmake to pull the library into place as a vtk module

vtk_module_export_info()
add_subdirectory(vtkxdmf3)

vtk_target_export(vtkxdmf3)
vtk_target_export(XdmfCore)
