/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPythonCommand.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkPythonCommand_h
#define vtkPythonCommand_h

#include "vtkWrappingPythonCoreModule.h" // For export macro
#include "vtkPython.h"
#include "vtkCommand.h"

// To allow Python to use the vtkCommand features
class VTKWRAPPINGPYTHONCORE_EXPORT vtkPythonCommand : public vtkCommand
{
public:
  vtkTypeMacro(vtkPythonCommand,vtkCommand);

  static vtkPythonCommand *New() { return new vtkPythonCommand; };

  void SetObject(PyObject *o);
  void SetThreadState(PyThreadState *ts);
  void Execute(vtkObject *ptr, unsigned long eventtype, void *callData) VTK_OVERRIDE;

  PyObject *obj;
  PyThreadState *ThreadState;
protected:
  vtkPythonCommand();
  ~vtkPythonCommand();
};

#endif
// VTK-HeaderTest-Exclude: vtkPythonCommand.h
