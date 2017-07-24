include(vtkQt)

# set up sources to build
set(LibSrcs
  vtkQtSQLDatabase.cxx
  vtkQtSQLQuery.cxx
  vtkQtTimePointUtility.cxx
  )

include_directories(${CMAKE_CURRENT_BINARY_DIR})

find_package(Qt5 COMPONENTS Widgets Sql REQUIRED QUIET)

# When this module is loaded by an app, load Qt too.
vtk_module_export_code_find_package(Qt5 COMPONENTS Sql Widgets)

set(${vtk-module}_NO_HeaderTest 1)
vtk_module_library(${vtk-module} ${LibSrcs})

# Apply user-defined properties to the library target.
if(VTK_LIBRARY_PROPERTIES)
  set_target_properties(${vtk-module} PROPERTIES ${VTK_LIBRARY_PROPERTIES})
endif()

vtk_module_link_libraries(${vtk-module} LINK_PUBLIC Qt5::Widgets Qt5::Sql)
