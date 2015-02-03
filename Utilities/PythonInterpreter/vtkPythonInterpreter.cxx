/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPythonInterpreter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPython.h" // this must be the first include.
#include "vtkPythonInterpreter.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkWeakPointer.h"
#include "vtkPythonStdStreamCaptureHelper.h"

#include <vtksys/SystemTools.hxx>

#include <algorithm>
#include <string>
#include <vector>
#include <signal.h>

namespace
{
  class StringPool
    {
  public:
    std::vector<char*> Strings;
    ~StringPool()
      {
      for (size_t cc=0; cc < this->Strings.size(); cc++)
        {
        delete [] this->Strings[cc];
        }
      }
    };

  static std::vector<vtkWeakPointer<vtkPythonInterpreter> > GlobalInterpreters;
  static std::vector<std::string> PythonPaths;

  void NotifyInterpreters(unsigned long eventid, void *calldata=NULL)
    {
    std::vector<vtkWeakPointer<vtkPythonInterpreter> >::iterator iter;
    for (iter = GlobalInterpreters.begin(); iter != GlobalInterpreters.end();
      ++iter)
      {
      if (iter->GetPointer())
        {
        iter->GetPointer()->InvokeEvent(eventid, calldata);
        }
      }
    }

  inline void vtkPrependPythonPath(const char* pathtoadd)
    {
    PyObject* path = PySys_GetObject(const_cast<char*>("path"));
    PyObject* newpath = PyString_FromString(pathtoadd);
    PyList_Insert(path, 0, newpath);
    Py_DECREF(newpath);
    }
}

bool vtkPythonInterpreter::InitializedOnce = false;
bool vtkPythonInterpreter::CaptureStdin = false;
bool vtkPythonInterpreter::ConsoleBuffering = false;
std::string vtkPythonInterpreter::StdErrBuffer;
std::string vtkPythonInterpreter::StdOutBuffer;


vtkStandardNewMacro(vtkPythonInterpreter);
//----------------------------------------------------------------------------
vtkPythonInterpreter::vtkPythonInterpreter()
{
  GlobalInterpreters.push_back(this);
}

//----------------------------------------------------------------------------
vtkPythonInterpreter::~vtkPythonInterpreter()
{
  std::vector<vtkWeakPointer<vtkPythonInterpreter> >::iterator iter;
  for (iter = GlobalInterpreters.begin(); iter != GlobalInterpreters.end();
    ++iter)
    {
    if (*iter == this)
      {
      GlobalInterpreters.erase(iter);
      break;
      }
    }
}

//----------------------------------------------------------------------------
bool vtkPythonInterpreter::IsInitialized()
{
  return (Py_IsInitialized() != 0);
}

//----------------------------------------------------------------------------
void vtkPythonInterpreter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
bool vtkPythonInterpreter::Initialize(int initsigs /*=0*/)
{
  vtkPythonInterpreter::InitializedOnce = true;
  if (Py_IsInitialized() == 0)
    {
#if (VTK_PYTHON_MAJOR_VERSION > 2) ||\
    (VTK_PYTHON_MAJOR_VERSION == 2 && VTK_PYTHON_MINOR_VERSION >= 4)
    Py_InitializeEx(initsigs);
#else
    (void)initsigs;
    Py_Initialize();
#endif

#ifdef SIGINT
    // Put default SIGINT handler back after Py_Initialize/Py_InitializeEx.
    signal(SIGINT, SIG_DFL);
#endif
    // HACK: Calling PyRun_SimpleString for the first time for some reason results in
    // a "\n" message being generated which is causing the error dialog to
    // popup. So we flush that message out of the system before setting up the
    // callbacks.
    vtkPythonInterpreter::RunSimpleString("");

    // Setup handlers for stdout/stdin/stderr.
    vtkPythonStdStreamCaptureHelper* wrapperOut =
      NewPythonStdStreamCaptureHelper(false);
    vtkPythonStdStreamCaptureHelper* wrapperErr =
      NewPythonStdStreamCaptureHelper(true);

    // Redirect Python's stdout and stderr and stdin
    PySys_SetObject(const_cast<char*>("stdout"),
      reinterpret_cast<PyObject*>(wrapperOut));
    PySys_SetObject(const_cast<char*>("stderr"),
      reinterpret_cast<PyObject*>(wrapperErr));
    PySys_SetObject(const_cast<char*>("stdin"),
      reinterpret_cast<PyObject*>(wrapperOut));
    Py_DECREF(wrapperOut);
    Py_DECREF(wrapperErr);

    for (size_t cc=0; cc < PythonPaths.size(); cc++)
      {
      vtkPrependPythonPath(PythonPaths[cc].c_str());
      }

    NotifyInterpreters(vtkCommand::EnterEvent);
    return true;
    }

  return false;
}

//----------------------------------------------------------------------------
void vtkPythonInterpreter::Finalize()
{
  if (Py_IsInitialized() != 0)
    {
    NotifyInterpreters(vtkCommand::ExitEvent);
    Py_Finalize();
    }
}

//----------------------------------------------------------------------------
void vtkPythonInterpreter::SetProgramName(const char* programname)
{
  if (vtkPythonInterpreter::InitializedOnce ||
    Py_IsInitialized() != 0)
    {
    return;
    }

  if (programname)
    {
    static StringPool pool;
    pool.Strings.push_back(vtksys::SystemTools::DuplicateString(programname));

    // From Python Docs: The argument should point to a zero-terminated character
    // string in static storage whose contents will not change for the duration of
    // the program's execution. No code in the Python interpreter will change the
    // contents of this storage.
    Py_SetProgramName(pool.Strings.back());
    }
}

//----------------------------------------------------------------------------
void vtkPythonInterpreter::PrependPythonPath(const char* dir)
{
  if (!dir) { return; }

  std::string out_dir = dir;

#if defined(_WIN32) && !defined(__CYGWIN__)
  // Convert slashes for this platform.
  std::replace(out_dir.begin(), out_dir.end(), '/', '\\');
#endif

  // save path for future use.
  PythonPaths.push_back(out_dir);

  if (Py_IsInitialized() == 0)
    {
    return;
    }

  // Append the path to the python sys.path object.
  vtkPrependPythonPath(out_dir.c_str());
}

//----------------------------------------------------------------------------
int vtkPythonInterpreter::PyMain(int argc, char** argv)
{
  if (!vtkPythonInterpreter::InitializedOnce && Py_IsInitialized() == 0 &&
    argc > 0)
    {
    vtkPythonInterpreter::SetProgramName(argv[0]);
    }
  vtkPythonInterpreter::Initialize(1);
  return Py_Main(argc, argv);
}

//----------------------------------------------------------------------------
void vtkPythonInterpreter::RunSimpleString(const char* script)
{
  vtkPythonInterpreter::Initialize(1);
  vtkPythonInterpreter::ConsoleBuffering = true;

  // The embedded python interpreter cannot handle DOS line-endings, see
  // http://sourceforge.net/tracker/?group_id=5470&atid=105470&func=detail&aid=1167922
  std::string buffer = script ? script : "";
  buffer.erase(std::remove(buffer.begin(), buffer.end(), '\r'), buffer.end());

  // The cast is necessary because PyRun_SimpleString() hasn't always been const-correct
  PyRun_SimpleString(const_cast<char*>(buffer.c_str()));
  vtkPythonInterpreter::ConsoleBuffering = false;
  if (! vtkPythonInterpreter::StdErrBuffer.empty())
    {
    NotifyInterpreters(vtkCommand::ErrorEvent, const_cast<char*>(
                         vtkPythonInterpreter::StdErrBuffer.c_str()));
    vtkPythonInterpreter::StdErrBuffer.clear();
    }
  if (! vtkPythonInterpreter::StdOutBuffer.empty())
    {
    NotifyInterpreters(vtkCommand::SetOutputEvent, const_cast<char*>(
                         vtkPythonInterpreter::StdOutBuffer.c_str()));
    vtkPythonInterpreter::StdOutBuffer.clear();
    }
}

//----------------------------------------------------------------------------
void vtkPythonInterpreter::SetCaptureStdin(bool val)
{
  vtkPythonInterpreter::CaptureStdin = val;
}

//----------------------------------------------------------------------------
bool vtkPythonInterpreter::GetCaptureStdin()
{
  return vtkPythonInterpreter::CaptureStdin;
}

//----------------------------------------------------------------------------
void vtkPythonInterpreter::WriteStdOut(const char* txt)
{
  cout << txt;
  if (vtkPythonInterpreter::ConsoleBuffering)
    {
    vtkPythonInterpreter::StdOutBuffer += std::string(txt);
    }
  else
    {
    NotifyInterpreters(vtkCommand::SetOutputEvent, const_cast<char*>(txt));
    }
}

//----------------------------------------------------------------------------
void vtkPythonInterpreter::FlushStdOut()
{
}

//----------------------------------------------------------------------------
void vtkPythonInterpreter::WriteStdErr(const char* txt)
{
  cerr << txt;
  if (vtkPythonInterpreter::ConsoleBuffering)
    {
    vtkPythonInterpreter::StdErrBuffer += std::string(txt);
    }
  else
    {
    NotifyInterpreters(vtkCommand::ErrorEvent, const_cast<char*>(txt));
    }
}

//----------------------------------------------------------------------------
void vtkPythonInterpreter::FlushStdErr()
{
}

//----------------------------------------------------------------------------
vtkStdString vtkPythonInterpreter::ReadStdin()
{
  if (!vtkPythonInterpreter::CaptureStdin)
    {
    vtkStdString string;
    cin >> string;
    return string;
    }
  vtkStdString string;
  NotifyInterpreters(vtkCommand::UpdateEvent, &string);
  return string;
}
