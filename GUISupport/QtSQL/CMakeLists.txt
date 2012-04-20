# set up sources to build
set(LibSrcs
  vtkQtSQLDatabase.cxx
  vtkQtSQLQuery.cxx
  vtkQtTimePointUtility.cxx
  )

include_directories(${CMAKE_CURRENT_BINARY_DIR})

# import Qt4 build settings
set(QT_USE_QTSQL 1)
find_package(Qt4 REQUIRED QUIET)
include(${QT_USE_FILE})

set(${vtk-module}_NO_HeaderTest 1)
vtk_module_library(${vtk-module} ${LibSrcs})

# Apply user-defined properties to the library target.
if(VTK_LIBRARY_PROPERTIES)
  set_target_properties(${vtk-module} PROPERTIES ${VTK_LIBRARY_PROPERTIES})
endif()

target_link_libraries(${vtk-module} ${QT_LIBRARIES})
