find_package(ODBC REQUIRED)

set(ODBC_SRCS
  vtkODBCDatabase.cxx
  vtkODBCQuery.cxx
  vtkIOODBC_AutoInit.cxx
  )

set_source_files_properties(vtkIOODBC_AutoInit WRAP_EXCLUDE)

include_directories(${ODBC_INCLUDE_DIRECTORIES})

vtk_module_library(vtkIOODBC ${ODBC_SRCS})

vtk_module_link_libraries(vtkIOODBC LINK_PRIVATE ${ODBC_LIBRARIES})
