set (Module_SRCS
  vtkPythonInterpreter.cxx
  vtkPythonInteractiveInterpreter.cxx
)

set (${vtk-module}_HDRS
  vtkPythonStdStreamCaptureHelper.h)

vtk_module_library(vtkPythonInterpreter
  ${Module_SRCS})
