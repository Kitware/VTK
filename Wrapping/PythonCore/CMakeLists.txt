set(VTK_WRAP_PYTHON_FIND_LIBS ON)
include(vtkWrapPython)

# Generate an extra export header for the python vtk command stuff
include(GenerateExportHeader)
generate_export_header(vtkCommonCorePythonD
  EXPORT_MACRO_NAME VTK_COMMON_PYTHON_EXPORT
  EXPORT_FILE_NAME vtkCommonCorePythonModule.h)

set(PythonCore_SRCS
  vtkPythonArgs.cxx
  vtkPythonOverload.cxx
  vtkPythonUtil.cxx
  PyVTKClass.cxx
  PyVTKMutableObject.cxx
  PyVTKObject.cxx
  PyVTKSpecialObject.cxx
  PyVTKTemplate.cxx
  )

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${VTK_SOURCE_DIR}/Common/Core
  ${VTK_BINARY_DIR}/Common/Core
  ${VTK_SOURCE_DIR}/Utilities
  ${VTK_BINARY_DIR}/Wrapping
  )

# SIP directory will be searched before other python dirs
if(VTK_WRAP_PYTHON_SIP)
  include_directories("${SIP_INCLUDE_DIR}")
endif()
include_directories("${PYTHON_INCLUDE_PATH}")

set(${vtk-module}_NO_HeaderTest 1)

vtk_module_library(vtkWrappingPythonCore ${PythonCore_SRCS})

target_link_libraries(vtkWrappingPythonCore
  ${VTK_PYTHON_LIBRARIES})
target_link_libraries(vtkWrappingPythonCore
  LINK_INTERFACE_LIBRARIES ${VTK_PYTHON_LIBRARIES})
