/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPythonInteractiveInterpreter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPythonInteractiveInterpreter - interpreter for interactive shells.
// .SECTION Description
// vtkPythonInteractiveInterpreter provides an interpreter that can be used in
// interactive shells. It mimicks the behaviour of the interactive
// console (much like the default Python shell) providing the "read-eval-print"
// loops. It also handles incomplete statements correctly. It uses "code"
// module provided by Python standard library to achieve this.
// It uses vtkPythonInterpreter to ensure that the global
// Python environment is setup correctly. Note that any time the
// vtkPythonInterpreter::Finalize() is called, the interactive interpreter will
// be destroyed as well. Subsequent calls to vtkPythonInterpreter::Push() will
// reinitialize Python as start a new interactive interpreter shell.
//
// This class also observers and forwards all events invoked by a
// vtkPythonInterpreter instance include vtkCommand::EnterEvent,
// vtkCommand::ExitEvent, vtkCommand::UpdateEvent, vtkCommand::ErrorEvent and
// vtkCommand::SetOutputEvent.

#ifndef __vtkPythonInteractiveInterpreter_h
#define __vtkPythonInteractiveInterpreter_h

#include "vtkObject.h"
#include "vtkPythonInterpreterModule.h" // For export macro

class vtkPythonInterpreter;

class VTKPYTHONINTERPRETER_EXPORT vtkPythonInteractiveInterpreter : public vtkObject
{
public:
  static vtkPythonInteractiveInterpreter* New();
  vtkTypeMacro(vtkPythonInteractiveInterpreter, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Push a line of code. It should have have trailing newlines. It can have
  // internal newlines. This can accept incomplete input. A command is executed
  // only after the complete input is received.  Look at Python module
  // documentation for code.InteractiveConsole.push() for further details.  The
  // return value is True if more input is required, False if the line was dealt
  // with in some way.
  bool Push(const char* const code);

  // Description:
  // This destroys the internal code.InteractiveConsole instance. Hence, next
  // time Push() will be called, it will use a brand new instance of
  // code.InteractiveConsole().
  void Reset();

  // Description:
  // Executes the given python source code using the context given by the
  // locals() object used by this interactive console.  This is similar to
  // using vtkPythonInterpreter::RunSimpleString(), except that method will
  // execute code in the context of the __main__ module. Returns 0 on success
  // or -1 if an exception was raised.
  int RunStringWithConsoleLocals(const char* script);

  // Description:
  // Provides access to the internal PyObject instances used for the
  // code.InteractiveConsole() as well as the dictionary for the locals of the
  // code.InteractiveConsole() instance. Do not use if you are not sure what
  // these are for.
  void* GetInteractiveConsolePyObject();
  void* GetInteractiveConsoleLocalsPyObject();

//BTX
protected:
  vtkPythonInteractiveInterpreter();
  ~vtkPythonInteractiveInterpreter();

  void HandleEvents(vtkObject* caller, unsigned long eventid, void* calldata);

private:
  vtkPythonInteractiveInterpreter(const vtkPythonInteractiveInterpreter&); // Not implemented.
  void operator=(const vtkPythonInteractiveInterpreter&); // Not implemented.

  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif
