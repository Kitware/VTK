find_package(ODBC REQUIRED)

set(ODBC_SRCS
  vtkODBCDatabase.cxx
  vtkODBCQuery.cxx
  vtkIOODBC_AutoInit.cxx
  )

set_source_files_properties(vtkIOODBC_AutoInit WRAP_EXCLUDE)

include_directories(${ODBC_INCLUDE_DIRECTORIES})

vtk_module_library(vtkIOODBC ${ODBC_SRCS})

target_link_libraries(vtkIOODBC ${ODBC_LIBRARIES})
