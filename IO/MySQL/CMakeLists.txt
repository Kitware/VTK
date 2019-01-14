set(classes
  vtkMySQLDatabase
  vtkMySQLQuery
  vtkMySQLToTableReader
  vtkTableToMySQLWriter)

set(sources
  vtkIOMySQL_AutoInit.cxx)

vtk_module_add_module(VTK::IOMySQL
  CLASSES ${classes}
  SOURCES ${sources})

vtk_module_find_package(PACKAGE MySQL)
vtk_module_link(VTK::IOMySQL
  PRIVATE
    MySQL::MySQL)
