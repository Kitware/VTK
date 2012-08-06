vtk_module_third_party(xdmf2
  LIBRARIES vtkxdmf2
  INCLUDE_DIRS
   ${CMAKE_CURRENT_SOURCE_DIR}/vtkxdmf2/libsrc
   ${CMAKE_CURRENT_BINARY_DIR}/vtkxdmf2/libsrc
  NO_ADD_SUBDIRECTORY
)

if(VTK_USE_SYSTEM_XDMF2)
  return()
endif()

#########################################################################
# Set Xdmf build environment options the way VTK wants them

# build options
option(VTK_XDMF_USE_MPI "Build Xdmf with MPI" OFF)
mark_as_advanced(VTK_XDMF_USE_MPI)
set (XDMF_BUILD_MPI ${VTK_XDMF_USE_MPI})

# wrapping
# VTK doesn't wrap libxdmf itself (either with vtk's wrapping or swig)
set(XDMF_WRAP_TCL OFF CACHE INTERNAL "")
set(XDMF_WRAP_PYTHON OFF CACHE INTERNAL "")
set(XDMF_WRAP_PYTHON_INSTALL_DIR ${VTK_INSTALL_LIBRARY_DIR}/site-packages/Xdmf)
set(XDMF_WRAP_CSHARP OFF CACHE INTERNAL "")
set(XDMF_REGENERATE_WRAPPERS OFF CACHE INTERNAL "")
set(XDMF_REGENERATE_YACCLEX OFF CACHE INTERNAL "")

# packaging
set(XDMF_INSTALL_LIB_DIR ${VTK_INSTALL_LIBRARY_DIR})
set(XDMF_INSTALL_BIN_DIR ${VTK_INSTALL_RUNTIME_DIR})
set(XDMF_INSTALL_INCLUDE_DIR "${VTK_INSTALL_INCLUDE_DIR}/vtkxdmf2")
set(XDMF_INSTALL_EXPORT_NAME ${VTK_INSTALL_EXPORT_NAME})
set(XDMF_INSTALL_NO_DEVELOPMENT ${VTK_INSTALL_NO_DEVELOPMENT})
set(XDMF_INSTALL_NO_RUNTIME ${VTK_INSTALL_NO_RUNTIME})

vtk_module_export_info()

#now configure the libxdmf build per the above settings
set(XDMF_BUILT_INVTK 1)
add_subdirectory(vtkxdmf2)

vtk_target(vtkxdmf2 NO_INSTALL)
