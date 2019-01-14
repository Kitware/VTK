vtk_module_find_package(PACKAGE PostgreSQL)

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
