set(classes
  vtkODBCDatabase
  vtkODBCQuery)

vtk_module_add_module(VTK::IOODBC
  CLASSES ${classes}
  SOURCES vtkIOODBC_AutoInit.cxx)

vtk_module_find_package(PACKAGE ODBC)
vtk_module_link(VTK::IOODBC
  PRIVATE
    ODBC::ODBC)
