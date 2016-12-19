# For now, load the python libraries through vtkWrapPython.cmake (eventually,
# FindPythonLibs.cmake should be fixed so it can be used here directly)
set(VTK_WRAP_PYTHON_FIND_LIBS ON)
include(vtkWrapPython)

# Check minimum versions of Python
set(_message "Python ${PYTHON_MAJOR_VERSION}.${PYTHON_MINOR_VERSION} is too old, use Python 2.7 or 3.3+")
set(_warning "Python ${PYTHON_MAJOR_VERSION}.${PYTHON_MINOR_VERSION} support is deprecated, use Python 2.7 or 3.3+")
if(PYTHON_MAJOR_VERSION EQUAL 3)
  if(PYTHON_MINOR_VERSION LESS 2)
    message(FATAL_ERROR ${_message})
  elseif(NOT VTK_LEGACY_SILENT AND PYTHON_MINOR_VERSION LESS 3)
    message(WARNING ${_warning})
  endif()
else()
  if(PYTHON_MINOR_VERSION LESS 6)
    message(FATAL_ERROR ${_message})
  elseif(NOT VTK_LEGACY_SILENT AND PYTHON_MINOR_VERSION LESS 7)
    message(WARNING ${_warning})
  endif()
endif()

set(${vtk-module}_LIBRARIES ${PYTHON_LIBRARIES})
set(${vtk-module}_SYSTEM_INCLUDE_DIRS "${PYTHON_INCLUDE_DIRS}")
set(${vtk-module}_NO_Header_Test 1)
vtk_module_impl()

# Export location of python module dirs in install and build tree for every vtkpython module to use
# As long as those modules depend on vtkpython, they can retrieve and use these
if (NOT VTK_BUILD_PYTHON_MODULE_DIR)
  set(VTK_BUILD_PYTHON_MODULE_DIR
      "${VTK_BINARY_DIR}/Wrapping/Python"
      CACHE
      PATH "Directory where python modules will be built" FORCE)
endif()

if (NOT VTK_INSTALL_PYTHON_MODULE_DIR)
  set(VTK_INSTALL_PYTHON_MODULE_DIR
      "${VTK_INSTALL_LIBRARY_DIR}/python${PYTHON_MAJOR_VERSION}.${PYTHON_MINOR_VERSION}/site-packages"
      CACHE
      PATH "Directory where python modules will be installed" FORCE)
endif()

# Export location of python module dirs in install and build tree for for
# projects that link against VTK
set(${vtk-module}_EXPORT_CODE_BUILD "set(VTK_PYTHONPATH \"${VTK_BUILD_PYTHON_MODULE_DIR}\")")
if(IS_ABSOLUTE ${VTK_INSTALL_PYTHON_MODULE_DIR})
  set(${vtk-module}_EXPORT_CODE_INSTALL "set(VTK_PYTHONPATH \"${VTK_INSTALL_PYTHON_MODULE_DIR}\")")
else()
  set(${vtk-module}_EXPORT_CODE_INSTALL "set(VTK_PYTHONPATH \"${CMAKE_INSTALL_PREFIX}/${VTK_INSTALL_PYTHON_MODULE_DIR}\")")
endif()

vtk_module_export_info()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/vtkPythonConfigure.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/vtkPythonConfigure.h)

set(${vtk-module}_HDRS
  vtkPython.h
  ${CMAKE_CURRENT_BINARY_DIR}/vtkPythonConfigure.h
  )

if(NOT VTK_INSTALL_NO_DEVELOPMENT)
  install(FILES ${${vtk-module}_HDRS}
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR}
    COMPONENT Development
    )
endif()
