# VTK_QT_VERSION is used to choose between Qt5 and Qt6.
# If it is set to Auto(default), VTK finds and uses the
# version installed on the system. If both versions are
# found, Qt6 is preferred.

set(vtk_supported_qt_versions "Auto" 5 6)

# The following `if` check can be removed once CMake 3.21 is required and
# the policy CMP0126 is set to NEW for ParaView and other superbuilds.
if (NOT DEFINED VTK_QT_VERSION)
  set(VTK_QT_VERSION "Auto" CACHE
    STRING "Expected Qt major version. Valid values are Auto, 5, 6.")
  set_property(CACHE VTK_QT_VERSION PROPERTY STRINGS "${vtk_supported_qt_versions}")
endif()

if (NOT VTK_QT_VERSION STREQUAL "Auto")
  if (NOT VTK_QT_VERSION IN_LIST vtk_supported_qt_versions)
    message(FATAL_ERROR
      "Supported Qt versions are \"${vtk_supported_qt_versions}\". But "
      "VTK_QT_VERSION is set to ${VTK_QT_VERSION}.")
  endif ()
  set(_vtk_qt_version "${VTK_QT_VERSION}")
else ()
  find_package(Qt6 QUIET COMPONENTS Core)
  set(_vtk_qt_version 6)
  if (NOT Qt6_FOUND)
    find_package(Qt5 QUIET COMPONENTS Core)
    if (NOT Qt5_FOUND)
      message(FATAL_ERROR
        "Could not find a valid Qt installation.")
    endif ()
    set(_vtk_qt_version 5)
  endif ()
endif ()
set(vtk_qt_major_version "${_vtk_qt_version}" CACHE INTERNAL
  "Major version number for the Qt installation used.")
