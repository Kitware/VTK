find_package(PostgreSQL REQUIRED)
mark_as_advanced(PostgreSQL_INCLUDE_DIR PostgreSQL_LIBRARY)

set(PostgreSQL_SRCS
  vtkPostgreSQLDatabase.cxx
  vtkPostgreSQLQuery.cxx
  vtkPostgreSQLToTableReader.cxx
  vtkTableToPostgreSQLWriter.cxx
  vtkIOPostgreSQL_AutoInit.cxx
  )

set_source_files_properties(vtkIOPostgreSQL_AutoInit WRAP_EXCLUDE)

include_directories(${PostgreSQL_INCLUDE_DIRS})

vtk_module_library(vtkIOPostgreSQL ${PostgreSQL_SRCS})

vtk_module_link_libraries(vtkIOPostgreSQL LINK_PRIVATE ${PostgreSQL_LIBRARY})
