find_package(MySQL REQUIRED)

set(MySQL_SRCS
  vtkMySQLDatabase.cxx
  vtkMySQLQuery.cxx
  vtkMySQLToTableReader.cxx
  vtkTableToMySQLWriter.cxx
  vtkIOMySQL_AutoInit.cxx
  )

include_directories(${MYSQL_INCLUDE_DIRECTORIES})

vtk_module_library(vtkIOMySQL ${MySQL_SRCS})

vtk_module_link_libraries(vtkIOMySQL LINK_PRIVATE ${MYSQL_LIBRARY} ${MYSQL_EXTRA_LIBRARIES})
