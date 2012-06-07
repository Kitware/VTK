set(VTK_WRAP_PYTHON_FIND_LIBS ON)
include(vtkWrapPython)
include(GenerateExportHeader)

# Python libraries (only the libraries, not the modules) are versioned.
set(XY ${PYTHON_MAJOR_VERSION}${PYTHON_MINOR_VERSION})

# Add a configure file for python
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/vtkPythonConfigure.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/vtkPythonConfigure.h)

set(PythonCore_SRCS
  vtkPythonArgs.cxx
  vtkPythonCommand.cxx
  vtkPythonOverload.cxx
  vtkPythonUtil.cxx
  PyVTKClass.cxx
  PyVTKMutableObject.cxx
  PyVTKObject.cxx
  PyVTKSpecialObject.cxx
  PyVTKTemplate.cxx
  )

set(PythonCore_HDRS
  vtkPython.h
  ${CMAKE_CURRENT_BINARY_DIR}/vtkPythonConfigure.h
  ${CMAKE_CURRENT_BINARY_DIR}/vtkPythonCoreModule.h
  )

if(NOT VTK_INSTALL_NO_DEVELOPMENT)
  set(_hdrs ${PythonCore_HDRS})
  foreach(src ${PythonCore_SRCS})
    get_filename_component(src_we ${src} NAME_WE)
    set(_hdrs ${_hdrs} ${src_we}.h)
  endforeach()
  install(FILES ${_hdrs}
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR}
    COMPONENT Development
    )
endif()

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${VTK_SOURCE_DIR}/Common/Core
  ${VTK_BINARY_DIR}/Common/Core
  ${VTK_SOURCE_DIR}/Utilities/KWSys
  ${VTK_BINARY_DIR}/Utilities/KWSys
  )

# SIP directory will be searched before other python dirs
if(VTK_WRAP_PYTHON_SIP)
  include_directories("${SIP_INCLUDE_DIR}")
endif()
include_directories("${PYTHON_INCLUDE_PATH}")

vtk_add_library(vtkPython${XY}Core ${PythonCore_SRCS})

target_link_libraries(vtkPython${XY}Core
  vtkCommonCore
  vtksys
  ${VTK_PYTHON_LIBRARIES})

target_link_libraries(vtkPython${XY}Core
  LINK_INTERFACE_LIBRARIES
  vtkCommonCore
  vtksys
  ${VTK_PYTHON_LIBRARIES})

# Generate an export header
generate_export_header(vtkPython${XY}Core
  BASE_NAME VTKPYTHONCORE
  EXPORT_FILE_NAME vtkPythonCoreModule.h)
