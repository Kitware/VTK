/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPythonInteractiveInterpreter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPython.h"

#include "vtkPythonInteractiveInterpreter.h"

#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkCommand.h"
#include "vtkPythonInterpreter.h"

#include <string>
class vtkPythonInteractiveInterpreter::vtkInternals
{
  PyObject* InteractiveConsole;
  PyObject* InteractiveConsoleLocals;

  std::string PS1;
  std::string PS2;
public:
  vtkSmartPointer<vtkPythonInterpreter> Interpreter;

  vtkInternals() : InteractiveConsole(0), InteractiveConsoleLocals(0) {}
  ~vtkInternals()
    {
    this->CleanupPythonObjects();
    }

  PyObject* GetInteractiveConsolePyObject()
     { return this->InteractiveConsole; }
  PyObject* GetInteractiveConsoleLocalsPyObject()
    { return this->InteractiveConsoleLocals; }

  void CleanupPythonObjects()
    {
    Py_XDECREF(this->InteractiveConsoleLocals);
    Py_XDECREF(this->InteractiveConsole);
    this->InteractiveConsole = NULL;
    this->InteractiveConsoleLocals = NULL;
    if (vtkPythonInterpreter::IsInitialized())
      {
      const char* code = "import gc; gc.collect()\n";
      vtkPythonInterpreter::RunSimpleString(code);
      }
    }

  PyObject* GetInteractiveConsole()
    {
    if (this->InteractiveConsole)
      {
      return this->InteractiveConsole;
      }

    vtkPythonInterpreter::Initialize();

    // set up the code.InteractiveConsole instance that we'll use.
    const char* code = "import code\n"
      "__vtkConsoleLocals={'__name__':'__vtkconsole__','__doc__':None}\n"
      "__vtkConsole=code.InteractiveConsole(__vtkConsoleLocals)\n";

    // The cast is necessary because PyRun_SimpleString() hasn't always been
    // const-correct
    PyRun_SimpleString(const_cast<char*>(code));

    // Now get the reference to __vtkConsole and save the pointer.
    PyObject* main_module = PyImport_AddModule((char*)"__main__");
    PyObject* global_dict = PyModule_GetDict(main_module);
    this->InteractiveConsole = PyDict_GetItemString(global_dict, "__vtkConsole");
    this->InteractiveConsoleLocals = PyDict_GetItemString(global_dict, "__vtkConsoleLocals");
    if (!this->InteractiveConsole || !this->InteractiveConsoleLocals)
      {
      vtkGenericWarningMacro(
        "Failed to locate the InteractiveConsole/InteractiveConsoleLocals object.");
      return NULL;
      }
    Py_INCREF(this->InteractiveConsole);
    Py_INCREF(this->InteractiveConsoleLocals);

    PyRun_SimpleString(
      const_cast<char*>("del __vtkConsole; del __vtkConsoleLocals"));

    // Maybe we need an API to enable developers to set the prompts.
    PyObject* ps1 = PySys_GetObject(const_cast<char*>("ps1"));
    if (!ps1)
      {
      PySys_SetObject(const_cast<char*>("ps1"), ps1 = PyString_FromString(">>> "));
      Py_XDECREF(ps1);
      }

    PyObject* ps2 = PySys_GetObject(const_cast<char*>("ps2"));
    if (!ps2)
      {
      PySys_SetObject(const_cast<char*>("ps2"), ps2 = PyString_FromString("... "));
      Py_XDECREF(ps2);
      }

    return this->InteractiveConsole;
    }
};

vtkStandardNewMacro(vtkPythonInteractiveInterpreter);
//----------------------------------------------------------------------------
vtkPythonInteractiveInterpreter::vtkPythonInteractiveInterpreter() :
  Internals(new vtkPythonInteractiveInterpreter::vtkInternals())
{
  this->Internals->Interpreter = vtkSmartPointer<vtkPythonInterpreter>::New();
  this->Internals->Interpreter->AddObserver(vtkCommand::AnyEvent,
    this, &vtkPythonInteractiveInterpreter::HandleEvents);
}

//----------------------------------------------------------------------------
vtkPythonInteractiveInterpreter::~vtkPythonInteractiveInterpreter()
{
  delete this->Internals;
  this->Internals = NULL;
}

//----------------------------------------------------------------------------
void vtkPythonInteractiveInterpreter::HandleEvents(
  vtkObject* vtkNotUsed(caller), unsigned long eventid, void* calldata)
{
  if (eventid == vtkCommand::ExitEvent)
    {
    this->Internals->CleanupPythonObjects();
    }

  this->InvokeEvent(eventid, calldata);
}

//----------------------------------------------------------------------------
bool vtkPythonInteractiveInterpreter::Push(const char* const code)
{
  PyObject* console = this->Internals->GetInteractiveConsole();
  if (!console)
    {
    return false;
    }

  // The embedded python interpreter cannot handle DOS line-endings, see
  // http://sourceforge.net/tracker/?group_id=5470&atid=105470&func=detail&aid=1167922
  std::string buffer = code ? code : "";
  // replace "\r\n" with "\n"
  std::string::size_type i = buffer.find("\r\n");
  for(; i != std::string::npos; i = buffer.find("\r\n", i))
    {
    buffer.replace(i, 2, "\n");
    i++;
    }

  // replace "\r" with "\n"  (sometimes seen on Mac)
  i = buffer.find("\r");
  for(; i != std::string::npos; i = buffer.find("\r", i))
    {
    buffer.replace(i, 1, "\n");
    i++;
    }

  bool ret_value = false;
  PyObject *res = PyObject_CallMethod(console,
    const_cast<char*>("push"), const_cast<char*>("z"), buffer.c_str());
  if (res)
    {
    int status = 0;
    if (PyArg_Parse(res, (char*)"i", &status))
      {
      ret_value = (status>0);
      }
    Py_DECREF(res);
    }
  return ret_value;
}

//----------------------------------------------------------------------------
int vtkPythonInteractiveInterpreter::RunStringWithConsoleLocals(const char* script)
{
  // The implementation of this method is modeled after
  // PyRun_SimpleStringFlags() found in Python's pythonrun.c

  this->Internals->GetInteractiveConsole(); //ensure the console is initialized

  PyObject* context = this->Internals->GetInteractiveConsoleLocalsPyObject();
  PyObject* result = PyRun_String(const_cast<char*>(script), Py_file_input, context, context);

  if (result == NULL)
    {
    PyErr_Print();
    return -1;
    }

  Py_DECREF(result);
  if (Py_FlushLine())
    {
    PyErr_Clear();
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkPythonInteractiveInterpreter::Reset()
{
  this->Internals->CleanupPythonObjects();
}

//----------------------------------------------------------------------------
void* vtkPythonInteractiveInterpreter::GetInteractiveConsolePyObject()
{
  return this->Internals->GetInteractiveConsolePyObject();
}

//----------------------------------------------------------------------------
void* vtkPythonInteractiveInterpreter::GetInteractiveConsoleLocalsPyObject()
{
  return this->Internals->GetInteractiveConsoleLocalsPyObject();
}

//----------------------------------------------------------------------------
void vtkPythonInteractiveInterpreter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
