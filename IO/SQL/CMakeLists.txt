set(classes
  vtkDatabaseToTableReader
  vtkRowQuery
  vtkRowQueryToTable
  vtkSQLDatabase
  #vtkSQLDatabaseGraphSource
  vtkSQLDatabaseSchema
  vtkSQLDatabaseTableSource
  #vtkSQLGraphReader
  vtkSQLiteDatabase
  vtkSQLiteQuery
  vtkSQLiteToTableReader
  vtkSQLQuery
  vtkTableToDatabaseWriter
  vtkTableToSQLiteWriter)

set(private_headers
  vtkSQLiteDatabaseInternals.h)

vtk_module_add_module(VTK::IOSQL
  CLASSES ${classes}
  PRIVATE_HEADERS ${private_headers})
