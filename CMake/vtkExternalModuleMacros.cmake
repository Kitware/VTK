# This file ensures the appropriate variables are set up for a project extending
# VTK before including vtkModuleMacros. This is the preferred way for a project
# building aginst VTK to use the CMake infrastructure provided for module
# developers.

if(NOT VTK_FOUND)
  message(FATAL_ERROR "VTK must be found before module macros can be used.")
endif()
if(VTK_VERSION VERSION_LESS "6.0")
  message(FATAL_ERROR "Requires VTK 6.0 or later to work.")
endif()

# VTK installation structure
if(NOT VTK_INSTALL_RUNTIME_DIR)
  set(VTK_INSTALL_RUNTIME_DIR bin)
endif()
if(NOT VTK_INSTALL_LIBRARY_DIR)
  set(VTK_INSTALL_LIBRARY_DIR lib)
endif()
if(NOT VTK_INSTALL_ARCHIVE_DIR)
  set(VTK_INSTALL_ARCHIVE_DIR lib)
endif()
if(NOT VTK_INSTALL_INCLUDE_DIR)
  set(VTK_INSTALL_INCLUDE_DIR include/vtk-${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION})
endif()
if(NOT VTK_INSTALL_DATA_DIR)
  set(VTK_INSTALL_DATA_DIR share/vtk-${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION})
endif()
if(NOT VTK_INSTALL_DOC_DIR)
  set(VTK_INSTALL_DOC_DIR share/doc/vtk-${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION})
endif()
if(NOT VTK_INSTALL_PACKAGE_DIR)
  set(VTK_INSTALL_PACKAGE_DIR "lib/cmake/vtk-${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}")
endif()
if(NOT VTK_INSTALL_DOXYGEN_DIR)
  set(VTK_INSTALL_DOXYGEN_DIR ${VTK_INSTALL_DOC_DIR}/doxygen)
endif()
if(NOT VTK_INSTALL_TCL_DIR)
  # tclsh searches <prefix>/lib/tcltk and its subdirectories for pkgIndex.tcl files
  set(VTK_INSTALL_TCL_DIR lib/tcltk/vtk-${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION})
endif()
if(NOT VTK_INSTALL_EXPORT_NAME)
  set(VTK_INSTALL_EXPORT_NAME VTKTargets)
endif()

# Add the VTK_MODULES_DIR to the CMAKE_MODULE_PATH and then use the binary
# directory for the project to write out new ones to.
if(VTK_MODULES_DIR)
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${VTK_MODULES_DIR})
endif()
set(VTK_MODULES_DIR "${CMAKE_BINARY_DIR}/${VTK_INSTALL_PACKAGE_DIR}/Modules")

include(vtkModuleMacros)
