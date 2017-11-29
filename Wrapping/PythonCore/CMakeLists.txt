include(VTKGenerateExportHeader)
include(vtkPythonWrapping)

set(Module_SRCS
  vtkPythonArgs.cxx
  vtkPythonCommand.cxx
  vtkPythonOverload.cxx
  vtkPythonUtil.cxx
  vtkSmartPyObject.cxx
  PyVTKExtras.cxx
  PyVTKMethodDescriptor.cxx
  PyVTKReference.cxx
  PyVTKNamespace.cxx
  PyVTKObject.cxx
  PyVTKSpecialObject.cxx
  PyVTKTemplate.cxx
  )

set(Module_HDRS
  vtkPythonCompatibility.h
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}Module.h
  )

list(APPEND Module_SRCS ${Module_HDRS})

set_source_files_properties(
  ${Module_HDRS}
  HEADER_FILE_ONLY
  )

include_directories(
  ${VTK_SOURCE_DIR}/Utilities
  )

set(PYTHON_WRAPPING_INCLUDE_DIRS)
set(${vtk-module}_SYSTEM_INCLUDE_DIRS ${PYTHON_WRAPPING_INCLUDE_DIRS})

set(${vtk-module}_NO_HeaderTest 1)
set(${vtk-module}_INCLUDE_DIRS)

set(XY ${PYTHON_MAJOR_VERSION}${PYTHON_MINOR_VERSION})
vtk_module_library(${vtk-module} ${Module_SRCS})
get_property(output_name TARGET ${vtk-module} PROPERTY OUTPUT_NAME)
string(REPLACE "Python" "Python${XY}" output_name "${output_name}")
set_property(TARGET ${vtk-module} PROPERTY OUTPUT_NAME ${output_name})
