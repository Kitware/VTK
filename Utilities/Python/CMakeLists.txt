if (NOT DEFINED VTK_PYTHON_VERSION)
  set(VTK_PYTHON_VERSION "3"
    CACHE STRING "")
  set_property(CACHE VTK_PYTHON_VERSION
    PROPERTY
      STRINGS "2;3")
endif ()

if (VTK_PYTHON_VERSION STREQUAL "2")
  set(vtk_python_min_version "2.6")
  set(vtk_python_version_support "2.7")
  if (VTK_REMOVE_LEGACY)
    set(vtk_python_min_version "${vtk_python_version_support}")
  endif ()
  vtk_module_find_package(
    PACKAGE Python2
    VERSION "${vtk_python_min_version}"
    COMPONENTS Development
    FORWARD_VERSION_REQ MINOR)
elseif (VTK_PYTHON_VERSION STREQUAL "3")
  set(vtk_python_min_version "3.2")
  set(vtk_python_version_support "3.3")
  if (VTK_REMOVE_LEGACY)
    set(vtk_python_min_version "${vtk_python_version_support}")
  endif ()
  vtk_module_find_package(
    PACKAGE Python3
    VERSION "${vtk_python_min_version}"
    COMPONENTS Development
    FORWARD_VERSION_REQ MINOR)
else ()
  message(FATAL_ERROR
    "`VTK_PYTHON_VERSION` must either be 2 or 3.")
endif ()
set(vtk_python_includes "${Python${VTK_PYTHON_VERSION}_INCLUDE_DIRS}")
set(vtk_python_target "Python${VTK_PYTHON_VERSION}::Python")
set(vtk_python_version "${Python${VTK_PYTHON_VERSION}_VERSION}")
set(vtk_python_version_pair "${Python${VTK_PYTHON_VERSION}_VERSION_MAJOR}.${Python${VTK_PYTHON_VERSION}_VERSION_MINOR}")

# Check deprecated versions of Python
if (NOT VTK_LEGACY_SILENT AND vtk_python_version VERSION_LESS vtk_python_version_support)
  message(DEPRECATION
    "Python ${vtk_python_version} support is deprecated, use Python ${vtk_python_version_support}+")
endif ()

# Export location of python module dirs in install and build tree for every vtkpython module to use
# As long as those modules depend on vtkpython, they can retrieve and use these
if (NOT VTK_PYTHON_SITE_PACKAGES_SUFFIX)
  if (WIN32 AND NOT CYGWIN)
    set(VTK_PYTHON_SITE_PACKAGES_SUFFIX "Lib/site-packages")
  else ()
    set(VTK_PYTHON_SITE_PACKAGES_SUFFIX
      "python${vtk_python_version_pair}/site-packages")
  endif ()
endif ()

if (CMAKE_CONFIGURATION_TYPES)
  # For build systems with configuration types e.g. Xcode/Visual Studio,
  # we rely on generator expressions.
  if (CMAKE_VERSION VERSION_LESS 3.4)
    message(FATAL_ERROR "CMake 3.4 or newer if needed for your generator.")
  endif ()
  set(VTK_BUILD_PYTHON_MODULES_DIR
    "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/$<CONFIG>/${VTK_PYTHON_SITE_PACKAGES_SUFFIX}"
    CACHE INTERNAL "Directory where python modules will be built")
else ()
  set(VTK_BUILD_PYTHON_MODULES_DIR
    "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${VTK_PYTHON_SITE_PACKAGES_SUFFIX}"
    CACHE INTERNAL "Directory where python modules will be built")
endif ()

if (NOT DEFINED VTK_INSTALL_PYTHON_MODULES_DIR)
  if (WIN32 AND NOT CYGWIN)
    set(VTK_INSTALL_PYTHON_MODULES_DIR
      "${VTK_INSTALL_RUNTIME_DIR}/${VTK_PYTHON_SITE_PACKAGES_SUFFIX}"
      CACHE INTERNAL "Directory where python modules will be installed")
  else ()
    set(VTK_INSTALL_PYTHON_MODULES_DIR
      "${VTK_INSTALL_LIBRARY_DIR}/${VTK_PYTHON_SITE_PACKAGES_SUFFIX}"
      CACHE INTERNAL "Directory where python modules will be installed")
  endif ()
endif ()

# Export location of python module dirs in install and build tree for for
# projects that link against VTK
set(${vtk-module}_EXPORT_CODE_BUILD "set(VTK_PYTHONPATH \"${VTK_BUILD_PYTHON_MODULES_DIR}\")")
if (IS_ABSOLUTE ${VTK_INSTALL_PYTHON_MODULES_DIR})
  set(${vtk-module}_EXPORT_CODE_INSTALL "set(VTK_PYTHONPATH \"${VTK_INSTALL_PYTHON_MODULES_DIR}\")")
else ()
  set(${vtk-module}_EXPORT_CODE_INSTALL "set(VTK_PYTHONPATH \"${CMAKE_INSTALL_PREFIX}/${VTK_INSTALL_PYTHON_MODULES_DIR}\")")
endif ()

# for static builds, since vtkPythonInterpreter cannot work off the library
# location, but instead has to use the executable location, it needs to know the
# library dir explicitly.
if (NOT VTK_BUILD_SHARED_LIBS)
  set(VTK_PYTHON_SITE_PACKAGES_SUFFIX_FIXED ${VTK_INSTALL_PYTHON_MODULES_DIR})
else ()
  set(VTK_PYTHON_SITE_PACKAGES_SUFFIX_FIXED ${VTK_PYTHON_SITE_PACKAGES_SUFFIX})
endif ()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkPythonConfigure.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkPythonConfigure.h")

set(headers
  vtkPython.h
  "${CMAKE_CURRENT_BINARY_DIR}/vtkPythonConfigure.h")

vtk_module_add_module(VTK::Python
  HEADERS ${headers}
  HEADER_ONLY)
vtk_module_link(VTK::Python
  INTERFACE
    "${vtk_python_target}")
