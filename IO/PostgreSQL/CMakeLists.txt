vtk_module_find_package(PRIVATE_IF_SHARED
  PACKAGE PostgreSQL)

set(classes
  vtkPostgreSQLDatabase
  vtkPostgreSQLQuery
  vtkPostgreSQLToTableReader
  vtkTableToPostgreSQLWriter)

vtk_module_add_module(VTK::IOPostgreSQL
  CLASSES ${classes}
  SOURCES vtkIOPostgreSQL_AutoInit.cxx)

vtk_module_link(VTK::IOPostgreSQL
  PRIVATE
    PostgreSQL::PostgreSQL)
vtk_add_test_mangling(VTK::IOPostgreSQL)
