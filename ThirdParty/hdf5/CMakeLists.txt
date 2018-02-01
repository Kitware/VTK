# The targets exported by HDF5 in its config mode do not work with the module
# system as-is.
set(HDF5_NO_FIND_PACKAGE_CONFIG_FILE ON)

vtk_module_third_party(HDF5
  LIBRARIES vtkhdf5_src vtkhdf5_hl_src
  INCLUDE_DIRS
    ${CMAKE_CURRENT_BINARY_DIR}/vtkhdf5
    ${CMAKE_CURRENT_SOURCE_DIR}/vtkhdf5/hl/src
    ${CMAKE_CURRENT_SOURCE_DIR}/vtkhdf5/src
  COMPONENTS C HL
  )

include(CMakeDependentOption)
cmake_dependent_option(VTK_MODULE_vtkhdf5_IS_SHARED "Whether the HDF5 in use is shared or not" ON
  VTK_USE_SYSTEM_HDF5 OFF)
mark_as_advanced(VTK_MODULE_vtkhdf5_IS_SHARED)
if (VTK_MODULE_vtkhdf5_IS_SHARED)
  set(H5_BUILT_AS_DYNAMIC_LIB 1)
endif ()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_hdf5.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_hdf5.h")

if (NOT VTK_INSTALL_NO_DEVELOPMENT)
  install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/vtk_hdf5.h"
    DESTINATION "${VTK_INSTALL_INCLUDE_DIR}"
    COMPONENT Development)
endif()
