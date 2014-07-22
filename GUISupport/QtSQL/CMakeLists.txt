include(vtkQt)

# set up sources to build
set(LibSrcs
  vtkQtSQLDatabase.cxx
  vtkQtSQLQuery.cxx
  vtkQtTimePointUtility.cxx
  )

include_directories(${CMAKE_CURRENT_BINARY_DIR})

if(VTK_QT_VERSION VERSION_GREATER "4")
  find_package(Qt5Widgets REQUIRED QUIET)
  find_package(Qt5Sql REQUIRED QUIET)

  set(_qt_include_dirs ${Qt5Widgets_INCLUDE_DIRS} ${Qt5Sql_INCLUDE_DIRS})
  list(REMOVE_DUPLICATES _qt_include_dirs)
  include_directories(${_qt_include_dirs})

  set(_qt_definitions ${Qt5Widgets_DEFINITIONS} ${Qt5Sql_DEFINITIONS})
  list(REMOVE_DUPLICATES _qt_definitions)
  add_definitions(${_qt_definitions})

  set(QT_LIBRARIES ${Qt5Widgets_LIBRARIES} ${Qt5Sql_LIBRARIES})

  # When this module is loaded by an app, load Qt too.
  vtk_module_export_code_find_package(Qt5Widgets)
  vtk_module_export_code_find_package(Qt5Sql)
else()
  # import Qt4 build settings
  set(QT_USE_QTSQL 1)
  find_package(Qt4 REQUIRED QUIET)
  include(${QT_USE_FILE})
endif()

set(${vtk-module}_NO_HeaderTest 1)
vtk_module_library(${vtk-module} ${LibSrcs})

# Apply user-defined properties to the library target.
if(VTK_LIBRARY_PROPERTIES)
  set_target_properties(${vtk-module} PROPERTIES ${VTK_LIBRARY_PROPERTIES})
endif()

vtk_module_link_libraries(${vtk-module} LINK_PRIVATE ${QT_LIBRARIES})
