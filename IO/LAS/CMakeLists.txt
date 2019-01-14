vtk_module_find_package(PACKAGE libLAS)

set(classes
  vtkLASReader)

vtk_module_add_module(VTK::IOLAS
  CLASSES ${classes})

vtk_module_link(VTK::IOLAS
  PRIVATE
    # XXX(liblas-1.8.2): Use these when the minimum version is 1.8.2+.
    # Currently, the imported target names vary based on the platform. In
    # addition, the include interface is not set up properly. See
    # https://github.com/libLAS/libLAS/pull/153.
    # las las_c
    ${libLAS_LIBRARIES})
vtk_module_include(VTK::IOLAS
  PRIVATE
    ${libLAS_INCLUDE_DIRS})
