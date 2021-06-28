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
#include "vtkPythonInterpreter.h"
#include "vtkPython.h" // this must be the first include.

#include "vtkCommand.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOutputWindow.h"
#include "vtkPythonStdStreamCaptureHelper.h"
#include "vtkResourceFileLocator.h"
#include "vtkVersion.h"
#include "vtkWeakPointer.h"

#include <vtksys/SystemInformation.hxx>
#include <vtksys/SystemTools.hxx>

#include <algorithm>
#include <csignal>
#include <sstream>
#include <string>
#include <vector>

#if PY_VERSION_HEX >= 0x03000000
#if defined(__APPLE__) && PY_VERSION_HEX < 0x03050000
extern "C"
{
  extern wchar_t* _Py_DecodeUTF8_surrogateescape(const char* s, Py_ssize_t size);
}
#endif
#endif

#if defined(_WIN32) && !defined(__CYGWIN__)
#define VTK_PATH_SEPARATOR "\\"
#else
#define VTK_PATH_SEPARATOR "/"
#endif

#define VTKPY_DEBUG_MESSAGE(x)                                                                     \
  vtkVLog(vtkLogger::ConvertToVerbosity(vtkPythonInterpreter::GetLogVerbosity()), x)
#define VTKPY_DEBUG_MESSAGE_VV(x)                                                                  \
  vtkVLog(vtkLogger::ConvertToVerbosity(vtkPythonInterpreter::GetLogVerbosity() + 1), x)

#if defined(_WIN32) && !defined(__CYGWIN__) && defined(VTK_BUILD_SHARED_LIBS) &&                   \
  PY_VERSION_HEX >= 0x03080000
#define vtkPythonInterpreter_USE_DIRECTORY_COOKIE
#endif

namespace
{

template <class T>
void strFree(T* foo)
{
  delete[] foo;
}

template <class T>
class PoolT
{
  std::vector<T*> Strings;

public:
  ~PoolT()
  {
    for (T* astring : this->Strings)
    {
      strFree(astring);
    }
  }

  T* push_back(T* val)
  {
    this->Strings.push_back(val);
    return val;
  }
};

using StringPool = PoolT<char>;
#if PY_VERSION_HEX >= 0x03000000
template <>
void strFree(wchar_t* foo)
{
#if PY_VERSION_HEX >= 0x03050000
  PyMem_RawFree(foo);
#else
  PyMem_Free(foo);
#endif
}
using WCharStringPool = PoolT<wchar_t>;
#endif

#if PY_VERSION_HEX >= 0x03000000
wchar_t* vtk_Py_DecodeLocale(const char* arg, size_t* size)
{
  (void)size;
#if PY_VERSION_HEX >= 0x03050000
  return Py_DecodeLocale(arg, size);
#elif defined(__APPLE__)
  return _Py_DecodeUTF8_surrogateescape(arg, strlen(arg));
#else
  return _Py_char2wchar(arg, size);
#endif
}
#endif

#if PY_VERSION_HEX >= 0x03000000
char* vtk_Py_EncodeLocale(const wchar_t* arg, size_t* size)
{
  (void)size;
#if PY_VERSION_HEX >= 0x03050000
  return Py_EncodeLocale(arg, size);
#else
  return _Py_wchar2char(arg, size);
#endif
}
#endif

static std::vector<vtkWeakPointer<vtkPythonInterpreter> >* GlobalInterpreters;
static std::vector<std::string> PythonPaths;

void NotifyInterpreters(unsigned long eventid, void* calldata = nullptr)
{
  std::vector<vtkWeakPointer<vtkPythonInterpreter> >::iterator iter;
  for (iter = GlobalInterpreters->begin(); iter != GlobalInterpreters->end(); ++iter)
  {
    if (iter->GetPointer())
    {
      iter->GetPointer()->InvokeEvent(eventid, calldata);
    }
  }
}

inline void vtkPrependPythonPath(const char* pathtoadd)
{
  VTKPY_DEBUG_MESSAGE("adding module search path " << pathtoadd);
  vtkPythonScopeGilEnsurer gilEnsurer;
  PyObject* path = PySys_GetObject(const_cast<char*>("path"));
#if PY_VERSION_HEX >= 0x03000000
  PyObject* newpath = PyUnicode_FromString(pathtoadd);
#else
  PyObject* newpath = PyString_FromString(pathtoadd);
#endif

  // avoid adding duplicate paths.
  if (PySequence_Contains(path, newpath) == 0)
  {
    PyList_Insert(path, 0, newpath);
  }
  Py_DECREF(newpath);
}

}

// Schwarz counter idiom for GlobalInterpreters object
static unsigned int vtkPythonInterpretersCounter;
vtkPythonGlobalInterpreters::vtkPythonGlobalInterpreters()
{
  if (vtkPythonInterpretersCounter++ == 0)
  {
    GlobalInterpreters = new std::vector<vtkWeakPointer<vtkPythonInterpreter> >();
  };
}

vtkPythonGlobalInterpreters::~vtkPythonGlobalInterpreters()
{
  if (--vtkPythonInterpretersCounter == 0)
  {
    delete GlobalInterpreters;
    GlobalInterpreters = nullptr;
  }
}

bool vtkPythonInterpreter::InitializedOnce = false;
bool vtkPythonInterpreter::CaptureStdin = false;
bool vtkPythonInterpreter::ConsoleBuffering = false;
std::string vtkPythonInterpreter::StdErrBuffer;
std::string vtkPythonInterpreter::StdOutBuffer;
int vtkPythonInterpreter::LogVerbosity = vtkLogger::VERBOSITY_TRACE;

vtkStandardNewMacro(vtkPythonInterpreter);
//----------------------------------------------------------------------------
vtkPythonInterpreter::vtkPythonInterpreter()
{
  GlobalInterpreters->push_back(this);
}

//----------------------------------------------------------------------------
vtkPythonInterpreter::~vtkPythonInterpreter()
{
  // We need to check that GlobalInterpreters has not been deleted yet. It can be
  // deleted prior to a call to this destructor if another static object with a
  // reference to a vtkPythonInterpreter object deletes that object after
  // GlobalInterpreters has been destructed. It all depends on the destruction order
  // of the other static object and GlobalInterpreters.
  if (!GlobalInterpreters)
  {
    return;
  }
  std::vector<vtkWeakPointer<vtkPythonInterpreter> >::iterator iter;
  for (iter = GlobalInterpreters->begin(); iter != GlobalInterpreters->end(); ++iter)
  {
    if (*iter == this)
    {
      GlobalInterpreters->erase(iter);
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
    // guide the mechanism to locate Python standard library, if possible.
    vtkPythonInterpreter::SetupPythonPrefix();

    Py_InitializeEx(initsigs);

    // setup default argv. Without this, code snippets that check `sys.argv` may
    // fail when run in embedded VTK Python environment.
    PySys_SetArgvEx(0, nullptr, 0);

#ifdef VTK_PYTHON_FULL_THREADSAFE
#if PY_VERSION_HEX < 0x03090000
    // In Python 3.9 and higher, PyEval_ThreadsInitialized() and
    // PyEval_InitThreads() are deprecated and do nothing.
    // GIL initialization is handled by Py_InitializeEx().
    int threadInit = PyEval_ThreadsInitialized();
    if (!threadInit)
    {
      PyEval_InitThreads(); // initialize and acquire GIL
    }
#endif
    // Always release GIL, as it has been acquired either by PyEval_InitThreads
    // prior to Python 3.7 or by Py_InitializeEx in Python 3.7 and after
    PyEval_SaveThread();
#endif

#ifdef SIGINT
    // Put default SIGINT handler back after Py_Initialize/Py_InitializeEx.
    signal(SIGINT, SIG_DFL);
#endif
  }

  if (!vtkPythonInterpreter::InitializedOnce)
  {
    vtkPythonInterpreter::InitializedOnce = true;

    // HACK: Calling PyRun_SimpleString for the first time for some reason results in
    // a "\n" message being generated which is causing the error dialog to
    // popup. So we flush that message out of the system before setting up the
    // callbacks.
    vtkPythonInterpreter::RunSimpleString("");

    // Redirect Python's stdout and stderr and stdin - GIL protected operation
    {
      // Setup handlers for stdout/stdin/stderr.
      vtkPythonStdStreamCaptureHelper* wrapperOut = NewPythonStdStreamCaptureHelper(false);
      vtkPythonStdStreamCaptureHelper* wrapperErr = NewPythonStdStreamCaptureHelper(true);
      vtkPythonScopeGilEnsurer gilEnsurer;
      PySys_SetObject(const_cast<char*>("stdout"), reinterpret_cast<PyObject*>(wrapperOut));
      PySys_SetObject(const_cast<char*>("stderr"), reinterpret_cast<PyObject*>(wrapperErr));
      PySys_SetObject(const_cast<char*>("stdin"), reinterpret_cast<PyObject*>(wrapperOut));
      Py_DECREF(wrapperOut);
      Py_DECREF(wrapperErr);
    }

    // We call this before processing any of Python paths added by the
    // application using `PrependPythonPath`. This ensures that application
    // specified paths are preferred to the ones `vtkPythonInterpreter` adds.
    vtkPythonInterpreter::SetupVTKPythonPaths();

    for (size_t cc = 0; cc < PythonPaths.size(); cc++)
    {
      vtkPrependPythonPath(PythonPaths[cc].c_str());
    }

    NotifyInterpreters(vtkCommand::EnterEvent);
    return true;
  }

  return false;
}

#ifdef vtkPythonInterpreter_USE_DIRECTORY_COOKIE
static PyObject* DLLDirectoryCookie = nullptr;

static void CloseDLLDirectoryCookie()
{
  if (DLLDirectoryCookie)
  {
    PyObject* close = PyObject_GetAttrString(DLLDirectoryCookie, "close");
    if (close)
    {
      PyObject* ret = PyObject_CallMethodObjArgs(DLLDirectoryCookie, close, nullptr);
      Py_XDECREF(ret);
    }

    Py_XDECREF(DLLDirectoryCookie);
    DLLDirectoryCookie = nullptr;
  }
}
#endif

//----------------------------------------------------------------------------
void vtkPythonInterpreter::Finalize()
{
  if (Py_IsInitialized() != 0)
  {
    NotifyInterpreters(vtkCommand::ExitEvent);
    vtkPythonScopeGilEnsurer gilEnsurer(false, true);
#ifdef vtkPythonInterpreter_USE_DIRECTORY_COOKIE
    CloseDLLDirectoryCookie();
#endif
    // Py_Finalize will take care of releasing gil
    Py_Finalize();
  }
}

//----------------------------------------------------------------------------
void vtkPythonInterpreter::SetProgramName(const char* programname)
{
  if (programname)
  {
// From Python Docs: The argument should point to a zero-terminated character
// string in static storage whose contents will not change for the duration of
// the program's execution. No code in the Python interpreter will change the
// contents of this storage.
#if PY_VERSION_HEX >= 0x03000000
    wchar_t* argv0 = vtk_Py_DecodeLocale(programname, nullptr);
    if (argv0 == 0)
    {
      fprintf(stderr,
        "Fatal vtkpython error: "
        "unable to decode the program name\n");
      static wchar_t empty[1] = { 0 };
      argv0 = empty;
      Py_SetProgramName(argv0);
    }
    else
    {
      static WCharStringPool wpool;
      Py_SetProgramName(wpool.push_back(argv0));
    }
#else
    static StringPool pool;
    Py_SetProgramName(pool.push_back(vtksys::SystemTools::DuplicateString(programname)));
#endif
  }
}

//----------------------------------------------------------------------------
void vtkPythonInterpreter::PrependPythonPath(const char* dir)
{
  if (!dir)
  {
    return;
  }

  std::string out_dir = dir;

#if defined(_WIN32) && !defined(__CYGWIN__)
  // Convert slashes for this platform.
  std::replace(out_dir.begin(), out_dir.end(), '/', '\\');
#endif

  if (Py_IsInitialized() == 0)
  {
    // save path for future use.
    PythonPaths.push_back(out_dir);
    return;
  }

  // Append the path to the python sys.path object.
  vtkPrependPythonPath(out_dir.c_str());
}

//----------------------------------------------------------------------------
void vtkPythonInterpreter::PrependPythonPath(
  const char* anchor, const char* landmark, bool add_landmark)
{
  const std::vector<std::string> prefixes = {
    VTK_PYTHON_SITE_PACKAGES_SUFFIX
#if defined(__APPLE__)
    // if in an App bundle, the `sitepackages` dir is <app_root>/Contents/Python
    ,
    "Contents/Python"
#endif
    ,
    "."
  };

  vtkNew<vtkResourceFileLocator> locator;
  locator->SetLogVerbosity(vtkPythonInterpreter::GetLogVerbosity() + 1);
  std::string path = locator->Locate(anchor, prefixes, landmark);
  if (!path.empty())
  {
    if (add_landmark)
    {
      path = path + "/" + landmark;
    }
    vtkPythonInterpreter::PrependPythonPath(path.c_str());
  }
}

//----------------------------------------------------------------------------
int vtkPythonInterpreter::PyMain(int argc, char** argv)
{
  vtksys::SystemTools::EnableMSVCDebugHook();

  int count_v = 0;
  for (int cc = 0; cc < argc; ++cc)
  {
    if (argv[cc] && strcmp(argv[cc], "-v") == 0)
    {
      ++count_v;
    }
    if (argv[cc] && strcmp(argv[cc], "-vv") == 0)
    {
      count_v += 2;
    }
  }

  if (count_v > 0)
  {
    // change the vtkPythonInterpreter's log verbosity. We only touch it
    // if the command line arguments explicitly requested a certain verbosity.
    vtkPythonInterpreter::SetLogVerbosity(vtkLogger::VERBOSITY_INFO);
    vtkLogger::SetStderrVerbosity(vtkLogger::ConvertToVerbosity(count_v - 1));
  }
  else
  {
    // update log verbosity such that default is to only show errors/warnings.
    // this avoids show the standard loguru INFO messages for executable args etc.
    // unless `-v` was specified.
    vtkLogger::SetStderrVerbosity(vtkLogger::VERBOSITY_WARNING);
  }

  vtkLogger::Init(argc, argv, nullptr); // since `-v` and `-vv` are parsed as Python verbosity flags
                                        // and not log verbosity flags.

  vtkPythonInterpreter::Initialize(1);

#if PY_VERSION_HEX >= 0x03000000

#if PY_VERSION_HEX >= 0x03070000 && PY_VERSION_HEX < 0x03080000
  // Python 3.7.0 has a bug where Py_InitializeEx (called above) followed by
  // Py_Main (at the end of this block) causes a crash. Gracefully exit with
  // failure if we're using 3.7.0 and suggest getting the newest 3.7.x release.
  // See <https://gitlab.kitware.com/vtk/vtk/issues/17434> for details.
  {
    bool is_ok = true;
    vtkPythonScopeGilEnsurer gilEnsurer(false, true);
    PyObject* sys = PyImport_ImportModule("sys");
    if (sys)
    {
      // XXX: Check sys.implementation.name == 'cpython'?

      PyObject* version_info = PyObject_GetAttrString(sys, "version_info");
      if (version_info)
      {
        PyObject* major = PyObject_GetAttrString(version_info, "major");
        PyObject* minor = PyObject_GetAttrString(version_info, "minor");
        PyObject* micro = PyObject_GetAttrString(version_info, "micro");

        auto py_number_cmp = [](PyObject* obj, long expected) {
          return obj && PyLong_Check(obj) && PyLong_AsLong(obj) == expected;
        };

        // Only 3.7.0 has this issue. Any failures to get the version
        // information is OK; we'll just crash later anyways if the version is
        // bad.
        is_ok = !py_number_cmp(major, 3) || !py_number_cmp(minor, 7) || !py_number_cmp(micro, 0);

        Py_XDECREF(micro);
        Py_XDECREF(minor);
        Py_XDECREF(major);
      }

      Py_XDECREF(version_info);
    }

    Py_XDECREF(sys);

    if (!is_ok)
    {
      std::cerr << "Python 3.7.0 has a known issue that causes a crash with a "
                   "specific API usage pattern. This has been fixed in 3.7.1 and all "
                   "newer 3.7.x Python releases. Exiting now to avoid the crash."
                << std::endl;
      return 1;
    }
  }
#endif

  // Need two copies of args, because programs might modify the first
  wchar_t** argvWide = new wchar_t*[argc];
  wchar_t** argvWide2 = new wchar_t*[argc];
  int argcWide = 0;
  for (int i = 0; i < argc; i++)
  {
    if (argv[i] && strcmp(argv[i], "--enable-bt") == 0)
    {
      vtksys::SystemInformation::SetStackTraceOnError(1);
      continue;
    }
    if (argv[i] && strcmp(argv[i], "-V") == 0)
    {
      // print out VTK version and let argument pass to Py_Main(). At which point,
      // Python will print its version and exit.
      cout << vtkVersion::GetVTKSourceVersion() << endl;
    }

    argvWide[argcWide] = vtk_Py_DecodeLocale(argv[i], nullptr);
    argvWide2[argcWide] = argvWide[argcWide];
    if (argvWide[argcWide] == 0)
    {
      fprintf(stderr,
        "Fatal vtkpython error: "
        "unable to decode the command line argument #%i\n",
        i + 1);
      for (int k = 0; k < argcWide; k++)
      {
        PyMem_Free(argvWide2[k]);
      }
      delete[] argvWide;
      delete[] argvWide2;
      return 1;
    }
    argcWide++;
  }
  vtkPythonScopeGilEnsurer gilEnsurer(false, true);
  int res = Py_Main(argcWide, argvWide);
  for (int i = 0; i < argcWide; i++)
  {
    PyMem_Free(argvWide2[i]);
  }
  delete[] argvWide;
  delete[] argvWide2;
  return res;
#else

  // process command line arguments to remove unhandled args.
  std::vector<char*> newargv;
  for (int i = 0; i < argc; ++i)
  {
    if (argv[i] && strcmp(argv[i], "--enable-bt") == 0)
    {
      vtksys::SystemInformation::SetStackTraceOnError(1);
      continue;
    }
    if (argv[i] && strcmp(argv[i], "-V") == 0)
    {
      // print out VTK version and let argument pass to Py_Main(). At which point,
      // Python will print its version and exit.
      cout << vtkVersion::GetVTKSourceVersion() << endl;
    }
    newargv.push_back(argv[i]);
  }

  vtkPythonScopeGilEnsurer gilEnsurer(false, true);
  return Py_Main(static_cast<int>(newargv.size()), &newargv[0]);
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
  if (!vtkPythonInterpreter::StdErrBuffer.empty())
  {
    vtkOutputWindow::GetInstance()->DisplayErrorText(vtkPythonInterpreter::StdErrBuffer.c_str());
    NotifyInterpreters(
      vtkCommand::ErrorEvent, const_cast<char*>(vtkPythonInterpreter::StdErrBuffer.c_str()));
    vtkPythonInterpreter::StdErrBuffer.clear();
  }
  if (!vtkPythonInterpreter::StdOutBuffer.empty())
  {
    vtkOutputWindow::GetInstance()->DisplayText(vtkPythonInterpreter::StdOutBuffer.c_str());
    NotifyInterpreters(
      vtkCommand::SetOutputEvent, const_cast<char*>(vtkPythonInterpreter::StdOutBuffer.c_str()));
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
  if (vtkPythonInterpreter::ConsoleBuffering)
  {
    vtkPythonInterpreter::StdOutBuffer += std::string(txt);
  }
  else
  {
    vtkOutputWindow::GetInstance()->DisplayText(txt);
    NotifyInterpreters(vtkCommand::SetOutputEvent, const_cast<char*>(txt));
  }
}

//----------------------------------------------------------------------------
void vtkPythonInterpreter::FlushStdOut() {}

//----------------------------------------------------------------------------
void vtkPythonInterpreter::WriteStdErr(const char* txt)
{
  if (vtkPythonInterpreter::ConsoleBuffering)
  {
    vtkPythonInterpreter::StdErrBuffer += std::string(txt);
  }
  else
  {
    vtkOutputWindow::GetInstance()->DisplayErrorText(txt);
    NotifyInterpreters(vtkCommand::ErrorEvent, const_cast<char*>(txt));
  }
}

//----------------------------------------------------------------------------
void vtkPythonInterpreter::FlushStdErr() {}

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

//----------------------------------------------------------------------------
void vtkPythonInterpreter::SetupPythonPrefix()
{
  using systools = vtksys::SystemTools;

  // Check Py_FrozenFlag global variable defined by Python to see if we're using
  // frozen Python.
  if (Py_FrozenFlag)
  {
    VTKPY_DEBUG_MESSAGE("`Py_FrozenFlag` is set. Skipping setting up of program path.");
    return;
  }

  std::string pythonlib = vtkGetLibraryPathForSymbol(Py_SetProgramName);
  if (pythonlib.empty())
  {
    VTKPY_DEBUG_MESSAGE("static Python build or `Py_SetProgramName` library couldn't be found. "
                        "Set `PYTHONHOME` if Python standard library fails to load.");
    return;
  }

  const std::string newprogramname =
    systools::GetFilenamePath(pythonlib) + VTK_PATH_SEPARATOR "vtkpython";
  VTKPY_DEBUG_MESSAGE(
    "calling Py_SetProgramName(" << newprogramname << ") to aid in setup of Python prefix.");
#if PY_VERSION_HEX >= 0x03000000
  static WCharStringPool wpool;
  Py_SetProgramName(wpool.push_back(vtk_Py_DecodeLocale(newprogramname.c_str(), nullptr)));
#else
  static StringPool pool;
  Py_SetProgramName(pool.push_back(systools::DuplicateString(newprogramname.c_str())));
#endif
}

//----------------------------------------------------------------------------
void vtkPythonInterpreter::SetupVTKPythonPaths()
{
  // Check Py_FrozenFlag global variable defined by Python to see if we're using
  // frozen Python.
  if (Py_FrozenFlag)
  {
    VTKPY_DEBUG_MESSAGE("`Py_FrozenFlag` is set. Skipping locating of `vtk` package.");
    return;
  }

  using systools = vtksys::SystemTools;
  std::string vtklib = vtkGetLibraryPathForSymbol(GetVTKVersion);
  if (vtklib.empty())
  {
    VTKPY_DEBUG_MESSAGE(
      "`GetVTKVersion` library couldn't be found. Will use `Py_GetProgramName` next.");
  }

  if (vtklib.empty())
  {
#if PY_VERSION_HEX >= 0x03000000
    auto tmp = vtk_Py_EncodeLocale(Py_GetProgramName(), nullptr);
    vtklib = tmp;
    PyMem_Free(tmp);
#else
    vtklib = Py_GetProgramName();
#endif
  }

  vtklib = systools::CollapseFullPath(vtklib);
  const std::string vtkdir = systools::GetFilenamePath(vtklib);

#if defined(_WIN32) && !defined(__CYGWIN__) && defined(VTK_BUILD_SHARED_LIBS)
  // On Windows, based on how the executable is run, we end up failing to load
  // pyd files due to inability to load dependent dlls. This seems to overcome
  // the issue.
  if (!vtkdir.empty())
  {
#if PY_VERSION_HEX >= 0x03080000
    vtkPythonScopeGilEnsurer gilEnsurer(false, true);
    CloseDLLDirectoryCookie();
    PyObject* os = PyImport_ImportModule("os");
    if (os)
    {
      PyObject* add_dll_directory = PyObject_GetAttrString(os, "add_dll_directory");
      if (add_dll_directory && PyCallable_Check(add_dll_directory))
      {
        PyObject* newpath = PyUnicode_FromString(vtkdir.c_str());
        DLLDirectoryCookie = PyObject_CallFunctionObjArgs(add_dll_directory, newpath, nullptr);
        Py_XDECREF(newpath);
      }

      Py_XDECREF(add_dll_directory);
    }

    Py_XDECREF(os);
#else
    std::string env_path;
    if (systools::GetEnv("PATH", env_path))
    {
      env_path = vtkdir + ";" + env_path;
    }
    else
    {
      env_path = vtkdir;
    }
    systools::PutEnv(std::string("PATH=") + env_path);
#endif
  }
#endif

#if defined(VTK_BUILD_SHARED_LIBS)
  vtkPythonInterpreter::PrependPythonPath(vtkdir.c_str(), "vtkmodules/__init__.py");
#else
  // since there may be other packages not zipped (e.g. mpi4py), we added path to _vtk.zip
  // to the search path as well.
  vtkPythonInterpreter::PrependPythonPath(vtkdir.c_str(), "_vtk.zip", /*add_landmark*/ false);
  vtkPythonInterpreter::PrependPythonPath(vtkdir.c_str(), "_vtk.zip", /*add_landmark*/ true);
#endif
}

//----------------------------------------------------------------------------
void vtkPythonInterpreter::SetLogVerbosity(int val)
{
  vtkPythonInterpreter::LogVerbosity = vtkLogger::ConvertToVerbosity(val);
}

//----------------------------------------------------------------------------
int vtkPythonInterpreter::GetLogVerbosity()
{
  return vtkPythonInterpreter::LogVerbosity;
}

#if !defined(VTK_LEGACY_REMOVE)
//----------------------------------------------------------------------------
int vtkPythonInterpreter::GetPythonVerboseFlag()
{
  VTK_LEGACY_REPLACED_BODY(
    vtkPythonInterpreter::GetPythonVerboseFlag, "VTK 9.0", vtkPythonInterpreter::GetLogVerbosity);
  return vtkPythonInterpreter::LogVerbosity == vtkLogger::VERBOSITY_INFO ? 1 : 0;
}
#endif
