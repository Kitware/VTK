// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPythonInterpreter.h"
#include "vtkPython.h" // this must be the first include.

#include "vtkBuild.h"
#include "vtkCommand.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOutputWindow.h"
#include "vtkPythonStdStreamCaptureHelper.h"
#include "vtkResourceFileLocator.h"
#include "vtkVersion.h"
#include "vtkWeakPointer.h"
#include "vtksys/Encoding.h"

#include <vtksys/Encoding.hxx>
#include <vtksys/SystemInformation.hxx>
#include <vtksys/SystemTools.hxx>

#include <algorithm>
#include <csignal>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

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
class PoolT
{
  std::vector<T*> Strings;

public:
  ~PoolT()
  {
    for (T* astring : this->Strings)
    {
      PyMem_RawFree(astring);
    }
  }

  T* push_back(T* val)
  {
    this->Strings.push_back(val);
    return val;
  }

  T* pop_last()
  {
    if (this->Strings.empty())
    {
      return nullptr;
    }
    T* last = *this->Strings.rbegin();
    this->Strings.pop_back();
    return last;
  }
};

using StringPool = PoolT<char>;
using WCharStringPool = PoolT<wchar_t>;

wchar_t* vtk_Py_UTF8ToWide(const char* arg)
{
  wchar_t* result = nullptr;
  if (arg != nullptr)
  {
    size_t length = vtksysEncoding_mbstowcs(nullptr, arg, 0);
    if (length > 0)
    {
      result = (wchar_t*)PyMem_RawMalloc(sizeof(wchar_t) * (length + 1));
      vtksysEncoding_mbstowcs(result, arg, length + 1);
    }
  }

  return result;
}

std::string vtk_Py_WideToUTF8(const wchar_t* arg)
{
  std::string result;
  size_t length = vtksysEncoding_wcstombs(nullptr, arg, 0);
  if (length > 0)
  {
    std::vector<char> chars(length + 1);
    vtksysEncoding_wcstombs(chars.data(), arg, length + 1);
    result.assign(chars.data(), length);
  }

  return result;
}

std::vector<vtkWeakPointer<vtkPythonInterpreter>>* GlobalInterpreters;
std::vector<std::string> PythonPaths;

void NotifyInterpreters(unsigned long eventid, void* calldata = nullptr)
{
  std::vector<vtkWeakPointer<vtkPythonInterpreter>>::iterator iter;
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
  PyObject* path = PySys_GetObject("path");
  PyObject* newpath = PyUnicode_FromString(pathtoadd);

  // avoid adding duplicate paths.
  if (PySequence_Contains(path, newpath) == 0)
  {
    PyList_Insert(path, 0, newpath);
  }
  Py_DECREF(newpath);
}
}

VTK_ABI_NAMESPACE_BEGIN
// Schwarz counter idiom for GlobalInterpreters object
static unsigned int vtkPythonInterpretersCounter;
vtkPythonGlobalInterpreters::vtkPythonGlobalInterpreters()
{
  if (vtkPythonInterpretersCounter++ == 0)
  {
    GlobalInterpreters = new std::vector<vtkWeakPointer<vtkPythonInterpreter>>();
  }
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
bool vtkPythonInterpreter::RedirectOutput = true;
bool vtkPythonInterpreter::ConsoleBuffering = false;
std::string vtkPythonInterpreter::StdErrBuffer;
std::string vtkPythonInterpreter::StdOutBuffer;
int vtkPythonInterpreter::LogVerbosity = vtkLogger::VERBOSITY_TRACE;
std::vector<void (*)()> vtkPythonInterpreter::AtExitCallbacks;

#if PY_VERSION_HEX >= 0x03000000
struct CharDeleter
{
  void operator()(wchar_t* str) { PyMem_RawFree(str); }
};
#endif

vtkStandardNewMacro(vtkPythonInterpreter);
//------------------------------------------------------------------------------
vtkPythonInterpreter::vtkPythonInterpreter()
{
  GlobalInterpreters->push_back(this);
}

//------------------------------------------------------------------------------
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
  std::vector<vtkWeakPointer<vtkPythonInterpreter>>::iterator iter;
  for (iter = GlobalInterpreters->begin(); iter != GlobalInterpreters->end(); ++iter)
  {
    if (*iter == this)
    {
      GlobalInterpreters->erase(iter);
      break;
    }
  }
}

//------------------------------------------------------------------------------
bool vtkPythonInterpreter::IsInitialized()
{
  return (Py_IsInitialized() != 0);
}

//------------------------------------------------------------------------------
void vtkPythonInterpreter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
bool vtkPythonInterpreter::Initialize(int initsigs /*=0*/)
{
  return vtkPythonInterpreter::InitializeWithArgs(initsigs, 0, nullptr);
}

#if PY_VERSION_HEX >= 0x03080000
static WCharStringPool PythonProgramName;
#endif

//------------------------------------------------------------------------------
// Ensure that Python is pre-initialized enough for VTK to do its
// initialization. Must be called before any `PyMem_*` calls are made.
static bool vtkPythonPreConfig()
{
  // Guard against doing this multiple times.
  static bool done = false;
  if (done)
  {
    return false;
  }
  done = true;

#if PY_VERSION_HEX >= 0x03080000
  PyStatus status;
  PyPreConfig preconfig;
  PyPreConfig_InitPythonConfig(&preconfig);

  preconfig.allocator = PYMEM_ALLOCATOR_NOT_SET;
  preconfig.utf8_mode = 1;

  status = Py_PreInitialize(&preconfig);
  if (PyStatus_Exception(status))
  {
    Py_ExitStatusException(status);
  }

  return preconfig.isolated;
#else
  return Py_FrozenFlag;
#endif
}

//------------------------------------------------------------------------------
namespace
{
/**
 * Since vtkPythonInterpreter is often used outside CPython executable, e.g.
 * vtkpython, the default logic to locate Python standard libraries used by
 * Python (which depends on the executable path) may fail or pickup incorrect
 * Python libs. This methods address the issue by setting program name to help
 * guide Python's default prefix/exec_prefix searching logic.
 */
void SetupPythonPrefix(bool isolated)
{
  using systools = vtksys::SystemTools;

  // Check if we're using an isolated Python.
  if (isolated)
  {
    VTKPY_DEBUG_MESSAGE("Isolated Python detected; skipping setting up of program path.");
    return;
  }

  std::string pythonlib = vtkGetLibraryPathForSymbol(Py_InitializeEx);
  if (pythonlib.empty())
  {
    VTKPY_DEBUG_MESSAGE("static Python build or `Py_InitializeEx` library couldn't be found. "
                        "Set `PYTHONHOME` if Python standard library fails to load.");
    return;
  }

  const std::string newprogramname =
    systools::GetFilenamePath(pythonlib) + VTK_PATH_SEPARATOR "vtkpython";
  VTKPY_DEBUG_MESSAGE("calling vtkPythonInterpreter::SetProgramName("
    << newprogramname << ") to aid in setup of Python prefix.");
  vtkPythonInterpreter::SetProgramName(newprogramname.c_str());
}

#ifdef vtkPythonInterpreter_USE_DIRECTORY_COOKIE
PyObject* DLLDirectoryCookie = nullptr;

void CloseDLLDirectoryCookie()
{
  if (DLLDirectoryCookie)
  {
    if (PyObject_HasAttrString(DLLDirectoryCookie, "close"))
    {
      PyObject* result = PyObject_CallMethod(DLLDirectoryCookie, "close", nullptr);
      Py_XDECREF(result);
    }
    Py_XDECREF(DLLDirectoryCookie);
    DLLDirectoryCookie = nullptr;
  }
}
#endif

//------------------------------------------------------------------------------
/**
 * Add paths to VTK's Python modules.
 */
void SetupVTKPythonPaths(bool isolated)
{
  // Check if we're using an isolated Python.
  if (isolated)
  {
    VTKPY_DEBUG_MESSAGE("Isolated Python detected; skipping setting up of `vtk` package.");
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
    vtklib = vtk_Py_WideToUTF8(Py_GetProgramName());
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
    vtkPythonScopeGilEnsurer gilEnsurer;
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
}

int vtkPythonInterpreter::AddAtExitCallback(void (*func)())
{
  if (Py_IsInitialized() == 0)
  {
    vtkPythonInterpreter::AtExitCallbacks.emplace_back(func);
    return 1;
  }

  return Py_AtExit(func);
}

//------------------------------------------------------------------------------
bool vtkPythonInterpreter::InitializeWithArgs(int initsigs, int argc, char* argv[])
{
  bool isolated = vtkPythonPreConfig();

  if (Py_IsInitialized() == 0)
  {
    // guide the mechanism to locate Python standard library, if possible.
    SetupPythonPrefix(isolated);
    bool signals_installed = initsigs != 0;

    // Need two copies of args, because programs might modify the first
    using OwnedWideString = std::unique_ptr<wchar_t, CharDeleter>;
    std::vector<wchar_t*> argvForPython;
    std::vector<OwnedWideString> argvCleanup;
    for (int i = 0; i < argc; i++)
    {
      OwnedWideString argCopy(vtk_Py_UTF8ToWide(argv[i]), CharDeleter());
      if (argCopy == nullptr)
      {
        fprintf(stderr,
          "Fatal vtkpython error: "
          "unable to decode the command line argument #%i\n",
          i + 1);
        return false;
      }

      argvForPython.push_back(argCopy.get());
      argvCleanup.emplace_back(std::move(argCopy));
    }
    argvForPython.push_back(nullptr);

#if PY_VERSION_HEX < 0x03080000
    Py_InitializeEx(initsigs);
    // setup default argv. Without this, code snippets that check `sys.argv` may
    // fail when run in embedded VTK Python environment.
    PySys_SetArgvEx(argc, argvForPython.data(), 0);

    isolated = Py_FrozenFlag;
#else
    PyConfig config;
    PyStatus status;
    PyConfig_InitPythonConfig(&config);
    config.install_signal_handlers = initsigs;
    config.program_name = PythonProgramName.pop_last();
    status = PyConfig_SetArgv(&config, argc, argvForPython.data());
    if (PyStatus_IsError(status))
    {
      PyConfig_Clear(&config);
      return false;
    }

    status = Py_InitializeFromConfig(&config);
    if (PyStatus_IsError(status))
    {
      PyConfig_Clear(&config);
      return false;
    }
    isolated = config.pathconfig_warnings == 0;
    PyConfig_Clear(&config);
#endif

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
    if (signals_installed)
    {
      // Put default SIGINT handler back after Py_Initialize/Py_InitializeEx.
      signal(SIGINT, SIG_DFL);
    }
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
    if (vtkPythonInterpreter::RedirectOutput)
    {
      // Setup handlers for stdout/stdin/stderr.
      vtkPythonStdStreamCaptureHelper* wrapperOut = NewPythonStdStreamCaptureHelper(false);
      vtkPythonStdStreamCaptureHelper* wrapperErr = NewPythonStdStreamCaptureHelper(true);
      vtkPythonScopeGilEnsurer gilEnsurer;
      PySys_SetObject("stdout", reinterpret_cast<PyObject*>(wrapperOut));
      PySys_SetObject("stderr", reinterpret_cast<PyObject*>(wrapperErr));
      PySys_SetObject("stdin", reinterpret_cast<PyObject*>(wrapperOut));
      Py_DECREF(wrapperOut);
      Py_DECREF(wrapperErr);
    }

    // We call this before processing any of Python paths added by the
    // application using `PrependPythonPath`. This ensures that application
    // specified paths are preferred to the ones `vtkPythonInterpreter` adds.
    SetupVTKPythonPaths(isolated);

    for (size_t cc = 0; cc < PythonPaths.size(); cc++)
    {
      vtkPrependPythonPath(PythonPaths[cc].c_str());
    }

    for (auto* func : vtkPythonInterpreter::AtExitCallbacks)
    {
      if (Py_AtExit(func))
      {
        return false;
      }
    }
    vtkPythonInterpreter::AtExitCallbacks.clear();

    NotifyInterpreters(vtkCommand::EnterEvent);
    return true;
  }

  return false;
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
void vtkPythonInterpreter::SetProgramName(const char* programname)
{
  vtkPythonPreConfig();
  if (programname)
  {
#if PY_VERSION_HEX >= 0x03080000
    if (wchar_t* argv0 = vtk_Py_UTF8ToWide(programname))
    {
      PythonProgramName.push_back(argv0);
    }
    else
    {
      fprintf(stderr,
        "Fatal vtkpython error: "
        "unable to decode the program name\n");
      wchar_t* empty = (wchar_t*)PyMem_RawMalloc(sizeof(wchar_t));
      empty[0] = 0;
      PythonProgramName.push_back(empty);
    }
#else
    // From Python Docs: The argument should point to a zero-terminated character
    // string in static storage whose contents will not change for the duration of
    // the program's execution. No code in the Python interpreter will change the
    // contents of this storage.
    wchar_t* argv0 = vtk_Py_UTF8ToWide(programname);
    if (argv0 == nullptr)
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
#endif
  }
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
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

  vtkLogger::Init(argc, argv, nullptr); // since `-v` and `-vv` are parsed as Python verbosity flags
                                        // and not log verbosity flags.

  // Need two copies of args, because the first array may be modified elsewhere.
  using OwnedCString = std::unique_ptr<char, decltype(&std::free)>;
  std::vector<char*> argvForPython;
  std::vector<OwnedCString> argvCleanup;
  for (int i = 0; i < argc; i++)
  {
    if (!argv[i])
    {
      continue;
    }
    if (strcmp(argv[i], "--enable-bt") == 0)
    {
      vtksys::SystemInformation::SetStackTraceOnError(1);
      continue;
    }
    if (strcmp(argv[i], "-V") == 0)
    {
      // print out VTK version and let argument pass to Py_RunMain(). At which
      // point, Python will print its version and exit.
      cout << vtkVersion::GetVTKSourceVersion() << endl;
    }

    OwnedCString argCopy(strdup(argv[i]), &std::free);
    if (argCopy == nullptr)
    {
      fprintf(stderr,
        "Fatal vtkpython error: "
        "unable to copy the command line argument #%i\n",
        i + 1);
      return 1;
    }

    argvForPython.push_back(argCopy.get());
    argvCleanup.emplace_back(std::move(argCopy));
  }
  int argvForPythonSize = static_cast<int>(argvForPython.size());
  argvForPython.push_back(nullptr);

  vtkPythonInterpreter::InitializeWithArgs(1, argvForPythonSize, argvForPython.data());

#if PY_VERSION_HEX >= 0x03070000 && PY_VERSION_HEX < 0x03080000
  // Python 3.7.0 has a bug where Py_InitializeEx (called above) followed by
  // Py_Main (at the end of this block) causes a crash. Gracefully exit with
  // failure if we're using 3.7.0 and suggest getting the newest 3.7.x release.
  // See <https://gitlab.kitware.com/vtk/vtk/-/issues/17434> for details.
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

#if PY_VERSION_HEX < 0x03080000
  // Need two copies of args, because programs might modify the first
  using OwnedWideString = std::unique_ptr<wchar_t, CharDeleter>;
  std::vector<wchar_t*> argvForPythonWide;
  std::vector<OwnedWideString> argvCleanupWide;
  for (size_t i = 0; i < argvCleanup.size(); i++)
  {
    OwnedWideString argCopy(vtk_Py_UTF8ToWide(argvCleanup[i].get()), CharDeleter());
    if (argCopy == nullptr)
    {
      fprintf(stderr,
        "Fatal vtkpython error: "
        "unable to decode the command line argument #%zu\n",
        i + 1);
      return 1;
    }

    argvForPythonWide.push_back(argCopy.get());
    argvCleanupWide.emplace_back(std::move(argCopy));
  }
  int argvForPythonWideSize = static_cast<int>(argvForPythonWide.size());
  argvForPythonWide.push_back(nullptr);

  vtkPythonScopeGilEnsurer gilEnsurer(false, true);
  return Py_Main(argvForPythonWideSize, argvForPythonWide.data());
#else
  vtkPythonScopeGilEnsurer gilEnsurer(false, true);
  return Py_RunMain();
#endif
}

//------------------------------------------------------------------------------
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
    pyReturn = PyRun_SimpleString(buffer.c_str());
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

//------------------------------------------------------------------------------
void vtkPythonInterpreter::SetCaptureStdin(bool val)
{
  vtkPythonInterpreter::CaptureStdin = val;
}

//------------------------------------------------------------------------------
bool vtkPythonInterpreter::GetCaptureStdin()
{
  return vtkPythonInterpreter::CaptureStdin;
}

//------------------------------------------------------------------------------
void vtkPythonInterpreter::SetRedirectOutput(bool redirect)
{
  vtkPythonInterpreter::RedirectOutput = redirect;
}

//------------------------------------------------------------------------------
bool vtkPythonInterpreter::GetRedirectOutput()
{
  return vtkPythonInterpreter::RedirectOutput;
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
void vtkPythonInterpreter::FlushStdOut() {}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
void vtkPythonInterpreter::FlushStdErr() {}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
void vtkPythonInterpreter::SetLogVerbosity(int val)
{
  vtkPythonInterpreter::LogVerbosity = vtkLogger::ConvertToVerbosity(val);
}

//------------------------------------------------------------------------------
int vtkPythonInterpreter::GetLogVerbosity()
{
  return vtkPythonInterpreter::LogVerbosity;
}

#if defined(_WIN32)
//------------------------------------------------------------------------------
vtkWideArgsConverter::vtkWideArgsConverter(int argc, wchar_t* wargv[])
{
  this->Argc = argc;
  for (int i = 0; i < argc; i++)
  {
    std::string str = vtksys::Encoding::ToNarrow(wargv[i]);
    char* cstr = vtksys::SystemTools::DuplicateString(str.c_str());
    Args.push_back(cstr);
    MemCache.push_back(cstr);
  }
  Args.push_back(nullptr);
}

//------------------------------------------------------------------------------
vtkWideArgsConverter::~vtkWideArgsConverter()
{
  for (auto cstr : MemCache)
  {
    delete[] cstr;
  }
}
#endif
VTK_ABI_NAMESPACE_END
