include(vtkQt)

# set up sources to build
set(LibSrcs
  vtkQtSQLDatabase.cxx
  vtkQtSQLQuery.cxx
  vtkQtTimePointUtility.cxx
  )

include_directories(${CMAKE_CURRENT_BINARY_DIR})

if(VTK_QT_VERSION VERSION_GREATER "4")
  find_package(Qt5 COMPONENTS Widgets REQUIRED QUIET)
  find_package(Qt5 COMPONENTS Sql REQUIRED QUIET)

  set(_qt_include_dirs ${Qt5Widgets_INCLUDE_DIRS} ${Qt5Sql_INCLUDE_DIRS})
  list(REMOVE_DUPLICATES _qt_include_dirs)
  include_directories(${_qt_include_dirs})

  set(_qt_definitions ${Qt5Widgets_DEFINITIONS} ${Qt5Sql_DEFINITIONS})
  list(REMOVE_DUPLICATES _qt_definitions)
  add_definitions(${_qt_definitions})

  set(QT_LIBRARIES ${Qt5Widgets_LIBRARIES} ${Qt5Sql_LIBRARIES})

  # When this module is loaded by an app, load Qt too.
  vtk_module_export_code_find_package(Qt5 COMPONENTS Sql Widgets)
else()
  # import Qt4 build settings
  find_package(Qt4 REQUIRED QtCore QtSql QUIET)
  include_directories(${QT_INCLUDES})
  add_definitions(${QT_DEFINITIONS})

  set(QT_LIBRARIES Qt4::QtCore Qt4::QtSql)

  # When this module is loaded by an app, load Qt too.
  vtk_module_export_code_find_package(Qt4 COMPONENTS QtCore QtSql)
endif()

set(${vtk-module}_NO_HeaderTest 1)
vtk_module_library(${vtk-module} ${LibSrcs})

# Apply user-defined properties to the library target.
if(VTK_LIBRARY_PROPERTIES)
  set_target_properties(${vtk-module} PROPERTIES ${VTK_LIBRARY_PROPERTIES})
endif()

vtk_module_link_libraries(${vtk-module} LINK_PUBLIC ${QT_LIBRARIES})
