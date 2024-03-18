vtk_module_find_package(PRIVATE_IF_SHARED
  PACKAGE Alembic
  VERSION 1.7)

set(classes
  vtkAlembicExporter)

vtk_module_add_module(VTK::IOAlembic
  CLASSES ${classes})

vtk_module_link(VTK::IOAlembic
  NO_KIT_EXPORT_IF_SHARED
  PRIVATE
    Alembic::Alembic)
vtk_add_test_mangling(VTK::IOAlembic)
