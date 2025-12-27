if (NOT VTK_MODULE_USE_EXTERNAL_VTK_eigen)
  message(FATAL_ERROR "Error: IFCReader requires an external eigen library. "
    "Set VTK_MODULE_USE_EXTERNAL_VTK_eigen and make sure you you compile with "
    "the same eigen library IfcOpenshell was compiled with.")
endif()

vtk_module_find_package(PRIVATE_IF_SHARED
  PACKAGE Eigen3)
add_library(Eigen3::Eigen ALIAS Eigen3::Eigen3)

vtk_module_find_package(PRIVATE_IF_SHARED
  PACKAGE Boost COMPONENTS ALL)

vtk_module_find_package(PRIVATE_IF_SHARED
  PACKAGE IfcOpenShell)


vtk_module_find_package(PRIVATE_IF_SHARED
  PACKAGE OpenCASCADE)

set(classes
  vtkIFCReader
)

vtk_module_add_module(VTK::IOIFC
  CLASSES ${classes}
  )
vtk_module_link(VTK::IOIFC
  NO_KIT_EXPORT_IF_SHARED
  PRIVATE
  IfcOpenShell::IfcParse
  IfcOpenShell::IfcGeom
  ${OpenCASCADE_LIBS}
)
vtk_module_include(VTK::IOIFC
  PRIVATE ${OpenCASCADE_INCLUDE_DIR})
vtk_add_test_mangling(VTK::IOIFC)
