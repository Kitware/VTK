# For now, load the python libraries through vtkWrapPython.cmake (eventually,
# FindPythonLibs.cmake should be fixed so it can be used here directly)
set(VTK_WRAP_PYTHON_FIND_LIBS ON)
include(vtkWrapPython)

set(${vtk-module}_LIBRARIES ${PYTHON_LIBRARIES})
set(${vtk-module}_SYSTEM_INCLUDE_DIRS "${PYTHON_INCLUDE_PATH}")
set(${vtk-module}_NO_Header_Test 1)
vtk_module_impl()
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
