// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkPythonCommand_h
#define vtkPythonCommand_h

#include "vtkABINamespace.h"
#include "vtkCommand.h"
#include "vtkPython.h"
#include "vtkWrappingPythonCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
// To allow Python to use the vtkCommand features
class VTKWRAPPINGPYTHONCORE_EXPORT vtkPythonCommand : public vtkCommand
{
public:
  vtkTypeMacro(vtkPythonCommand, vtkCommand);

  static vtkPythonCommand* New() { return new vtkPythonCommand; }

  void SetObject(PyObject* o);
  void SetThreadState(PyThreadState* ts);
  void Execute(vtkObject* ptr, unsigned long eventtype, void* callData) override;

  PyObject* obj;
  PyThreadState* ThreadState;

protected:
  vtkPythonCommand();
  ~vtkPythonCommand() override;
};
VTK_ABI_NAMESPACE_END

#endif
// VTK-HeaderTest-Exclude: vtkPythonCommand.h
