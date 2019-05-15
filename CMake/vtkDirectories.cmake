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
vtk_set_with_default(VTK_INSTALL_NDK_MODULES_DIR "${VTK_INSTALL_DATA_DIR}/ndk-modules")
