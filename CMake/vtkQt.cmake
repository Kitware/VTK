# VTK_QT_VERSION is used to choose between Qt5 and Qt6.
# If it is set to Auto(default), VTK finds and uses the
# version installed on the system. If both versions are
# found, Qt6 is preferred.

set(VTK_QT_VERSION "Auto" CACHE
  STRING "Expected Qt major version. Valid values are Auto, 5, 6.")
set(vtk_supported_qt_versions "Auto" 5 6)
set_property(CACHE VTK_QT_VERSION PROPERTY STRINGS ${vtk_supported_qt_versions})

if(NOT VTK_QT_VERSION STREQUAL "Auto")
  if(NOT VTK_QT_VERSION IN_LIST vtk_supported_qt_versions)
    message(FATAL_ERROR "Supported Qt versions are \"${vtk_supported_qt_versions}\"."
      " But VTK_QT_VERSION is set to ${VTK_QT_VERSION}.")
  endif()
  set(_vtk_qt_version ${VTK_QT_VERSION})
else()
  find_package(Qt6Widgets QUIET)
  set(_vtk_qt_version 6)
  if(NOT Qt6Widgets_FOUND)
    find_package(Qt5Widgets QUIET)
    if(NOT Qt5Widgets_FOUND)
      message(FATAL_ERROR "Could not find a valid Qt installation.")
    endif()
    set(_vtk_qt_version 5)
  endif()
endif()
set(VTK_QT_MAJOR_VERSION ${_vtk_qt_version} CACHE INTERNAL
  "Major version number for the Qt installation used.")
