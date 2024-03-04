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

vtk_module_find_package(PRIVATE_IF_SHARED
  PACKAGE MySQL)
vtk_module_link(VTK::IOMySQL
  NO_KIT_EXPORT_IF_SHARED
  PRIVATE
    MySQL::MySQL)
vtk_add_test_mangling(VTK::IOMySQL)
