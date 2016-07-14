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

#if PY_VERSION_HEX >= 0x03000000
#if defined(__APPLE__) && PY_VERSION_HEX < 0x03050000
extern "C" {
extern wchar_t*
_Py_DecodeUTF8_surrogateescape(const char *s, Py_ssize_t size);
}
#endif
#endif

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
    vtkPythonScopeGilEnsurer gilEnsurer;
    PyObject* path = PySys_GetObject(const_cast<char*>("path"));
#if PY_VERSION_HEX >= 0x03000000
    PyObject* newpath = PyUnicode_FromString(pathtoadd);
#else
    PyObject* newpath = PyString_FromString(pathtoadd);
#endif
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
    }

  if (!vtkPythonInterpreter::InitializedOnce)
    {
    vtkPythonInterpreter::InitializedOnce = true;

#ifdef VTK_PYTHON_FULL_THREADSAFE
    int threadInit = PyEval_ThreadsInitialized();
    PyEval_InitThreads(); // safe to call this multiple time
    if(!threadInit)
      {
      PyEval_SaveThread(); // release GIL
      }
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

    // Redirect Python's stdout and stderr and stdin - GIL protected operation
    {
    vtkPythonScopeGilEnsurer gilEnsurer;
    PySys_SetObject(const_cast<char*>("stdout"),
      reinterpret_cast<PyObject*>(wrapperOut));
    PySys_SetObject(const_cast<char*>("stderr"),
      reinterpret_cast<PyObject*>(wrapperErr));
    PySys_SetObject(const_cast<char*>("stdin"),
      reinterpret_cast<PyObject*>(wrapperOut));
    Py_DECREF(wrapperOut);
    Py_DECREF(wrapperErr);
    }

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
    vtkPythonScopeGilEnsurer gilEnsurer(false, true); 
    // Py_Finalize will take care of relasing gil
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
#if PY_VERSION_HEX >= 0x03000000
    wchar_t *argv0;
    const std::string& av0 = pool.Strings.back();
#if PY_VERSION_HEX >= 0x03050000
    argv0 = Py_DecodeLocale(av0.c_str(), NULL);
#elif defined(__APPLE__)
    argv0 = _Py_DecodeUTF8_surrogateescape(av0.data(), av0.length());
#else
    argv0 = _Py_char2wchar(av0.c_str(), NULL);
#endif
    if (argv0 == 0)
      {
      fprintf(stderr, "Fatal vtkpython error: "
                      "unable to decode the program name\n");
      static wchar_t empty[1] = { 0 };
      argv0 = empty;
      }
    Py_SetProgramName(argv0);
#else
    Py_SetProgramName(pool.Strings.back());
#endif
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

#if PY_VERSION_HEX >= 0x03000000
  wchar_t *argv0;
#if PY_VERSION_HEX >= 0x03050000
  argv0 = Py_DecodeLocale(argv[0], NULL);
#elif defined(__APPLE__)
  argv0 = _Py_DecodeUTF8_surrogateescape(argv[0], strlen(argv[0]));
#else
  argv0 = _Py_char2wchar(argv[0], NULL);
#endif
    if (argv0 == 0)
      {
      static wchar_t empty[1] = { 0 };
      argv0 = empty;
      }
  // Need two copies of args, because programs might modify the first
  wchar_t **argvWide = new wchar_t *[argc];
  wchar_t **argvWide2 = new wchar_t *[argc];
  for (int i = 0; i < argc; i++)
    {
#if PY_VERSION_HEX >= 0x03050000
    argvWide[i] = Py_DecodeLocale(argv[i], NULL);
#elif defined(__APPLE__)
    argvWide[i] = _Py_DecodeUTF8_surrogateescape(argv[i], strlen(argv[i]));
#else
    argvWide[i] = _Py_char2wchar(argv[i], NULL);
#endif
    argvWide2[i] = argvWide[i];
    if (argvWide[i] == 0)
      {
      fprintf(stderr, "Fatal vtkpython error: "
                      "unable to decode the command line argument #%i\n",
                      i + 1);
      for (int k = 0; k < i; k++)
        {
        PyMem_Free(argvWide2[i]);
        }
      PyMem_Free(argv0);
      delete [] argvWide;
      delete [] argvWide2;
      return 1;
      }
    }
  vtkPythonScopeGilEnsurer gilEnsurer;
  int res = Py_Main(argc, argvWide);
  PyMem_Free(argv0);
  for (int i = 0; i < argc; i++)
    {
    PyMem_Free(argvWide2[i]);
    }
  delete [] argvWide;
  delete [] argvWide2;
  return res;
#else

  vtkPythonScopeGilEnsurer gilEnsurer(false, true);
  return Py_Main(argc, argv);
#endif
}

//----------------------------------------------------------------------------
int vtkPythonInterpreter::RunSimpleString(const char* script)
{
  vtkPythonInterpreter::Initialize(1);
  vtkPythonInterpreter::ConsoleBuffering = true;

  // The embedded python interpreter cannot handle DOS line-endings, see
  // http://sourceforge.net/tracker/?group_id=5470&atid=105470&func=detail&aid=1167922
  std::string buffer = script ? script : "";
  buffer.erase(std::remove(buffer.begin(), buffer.end(), '\r'), buffer.end());

  // The cast is necessary because PyRun_SimpleString() hasn't always been const-correct
  int pyReturn;
    {
    vtkPythonScopeGilEnsurer gilEnsurer;
    pyReturn = PyRun_SimpleString(const_cast<char*>(buffer.c_str()));
    }

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

  return pyReturn;
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
    cout.flush();
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
    cerr.flush();
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
