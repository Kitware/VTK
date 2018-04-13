include(GNUInstallDirs)

# VTK installation structure
set(vtk_subdir "vtk-${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}")
vtk_set_with_default(VTK_INSTALL_RUNTIME_DIR "${CMAKE_INSTALL_BINDIR}")
vtk_set_with_default(VTK_INSTALL_LIBRARY_DIR "${CMAKE_INSTALL_LIBDIR}")
vtk_set_with_default(VTK_INSTALL_ARCHIVE_DIR "${CMAKE_INSTALL_LIBDIR}")
vtk_set_with_default(VTK_INSTALL_INCLUDE_DIR "${CMAKE_INSTALL_INCLUDEDIR}/${vtk_subdir}")
vtk_set_with_default(VTK_INSTALL_DATA_DIR "${CMAKE_INSTALL_DATADIR}/${vtk_subdir}")
# CMAKE_INSTALL_DOCDIR already includes PROJECT_NAME, which is not what we want
vtk_set_with_default(VTK_INSTALL_DOC_DIR "${CMAKE_INSTALL_DATAROOTDIR}/doc/${vtk_subdir}")
vtk_set_with_default(VTK_INSTALL_PACKAGE_DIR "${VTK_INSTALL_LIBRARY_DIR}/cmake/${vtk_subdir}")
vtk_set_with_default(VTK_INSTALL_DOXYGEN_DIR "${VTK_INSTALL_DOC_DIR}/doxygen")
vtk_set_with_default(VTK_INSTALL_EXPORT_NAME "VTKTargets")
vtk_set_with_default(VTK_INSTALL_NDK_MODULES_DIR "${VTK_INSTALL_DATA_DIR}/ndk-modules")

# Set up our directory structure for output libraries and binaries
vtk_set_with_default(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${VTK_BINARY_DIR}/bin")
if(UNIX)
  set(vtk_library_directory "lib")
else()
  set(vtk_library_directory "bin")
endif()
vtk_set_with_default(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${VTK_BINARY_DIR}/${vtk_library_directory}")
vtk_set_with_default(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${VTK_BINARY_DIR}/lib")
vtk_set_with_default(VTK_MODULES_DIR "${VTK_BINARY_DIR}/${VTK_INSTALL_PACKAGE_DIR}/Modules")
vtk_set_with_default(VTK_WWW_DIR "${VTK_BINARY_DIR}/www")

if(DEFINED VTK_INSTALL_PYTHON_MODULE_DIR)
  message(WARNING
    "VTK_INSTALL_PYTHON_MODULE_DIR is no longer supported. "
    "Set `VTK_PYTHON_SITE_PACKAGES_SUFFIX` instead, although not needed in most cases.")
endif()
if(DEFINED VTK_BUILD_PYTHON_MODULE_DIR)
  message(WARNING
    "VTK_BUILD_PYTHON_MODULE_DIR is no longer supported. "
    "Set `VTK_PYTHON_SITE_PACKAGES_SUFFIX` instead, although not needed in most cases.")
endif()
