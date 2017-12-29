set(classes
  vtkPythonInteractiveInterpreter
  vtkPythonInterpreter)

set(headers
  vtkPythonStdStreamCaptureHelper.h)

vtk_module_add_module(VTK::PythonInterpreter
  CLASSES ${classes}
  HEADERS ${headers})
