// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkResourceFileLocator.h"

#include "vtkLogger.h"
#include "vtkObjectFactory.h"

#include <vtksys/Encoding.hxx>
#include <vtksys/SystemTools.hxx>

#if defined(_WIN32) && !defined(__CYGWIN__)
#define VTK_PATH_SEPARATOR "\\"
#else
#define VTK_PATH_SEPARATOR "/"
#endif

#define VTK_FILE_LOCATOR_DEBUG_MESSAGE(...)                                                        \
  vtkVLogF(static_cast<vtkLogger::Verbosity>(this->LogVerbosity), __VA_ARGS__)

#if defined(_WIN32) && !defined(__CYGWIN__)
// Implementation for Windows win32 code but not cygwin
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

#if !defined(_WIN32) || defined(__CYGWIN__)
#include <limits.h>
#include <unistd.h>
#endif

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkResourceFileLocator);
//------------------------------------------------------------------------------
vtkResourceFileLocator::vtkResourceFileLocator()
  : LogVerbosity(vtkLogger::VERBOSITY_TRACE)
{
}

//------------------------------------------------------------------------------
vtkResourceFileLocator::~vtkResourceFileLocator() = default;

//------------------------------------------------------------------------------
std::string vtkResourceFileLocator::Locate(
  const std::string& anchor, const std::string& landmark, const std::string& defaultDir)
{
  return this->Locate(anchor, { std::string() }, landmark, defaultDir);
}

//------------------------------------------------------------------------------
std::string vtkResourceFileLocator::Locate(const std::string& anchor,
  const std::vector<std::string>& landmark_prefixes, const std::string& landmark,
  const std::string& defaultDir)
{
  vtkVLogScopeF(
    static_cast<vtkLogger::Verbosity>(this->LogVerbosity), "looking for '%s'", landmark.c_str());
  std::vector<std::string> path_components;
  vtksys::SystemTools::SplitPath(anchor, path_components);
  while (!path_components.empty())
  {
    std::string curanchor = vtksys::SystemTools::JoinPath(path_components);
    for (const std::string& curprefix : landmark_prefixes)
    {
      std::string landmarkdir =
        curprefix.empty() ? curanchor : curanchor + VTK_PATH_SEPARATOR + curprefix;
      const std::string landmarktocheck = landmarkdir + VTK_PATH_SEPARATOR + landmark;
      if (vtksys::SystemTools::FileExists(landmarktocheck))
      {
        VTK_FILE_LOCATOR_DEBUG_MESSAGE("trying file %s -- found!", landmarktocheck.c_str());
        return landmarkdir;
      }
      else
      {
        VTK_FILE_LOCATOR_DEBUG_MESSAGE("trying file %s -- not found!", landmarktocheck.c_str());
      }
    }
    path_components.pop_back();
  }
  return defaultDir;
}

//------------------------------------------------------------------------------
VTK_FILEPATH std::string vtkResourceFileLocator::GetLibraryPathForAddress(const void* ptr)
{
#if defined(_WIN32) && !defined(__CYGWIN__)
  MEMORY_BASIC_INFORMATION mbi;
  if (!VirtualQuery(ptr, &mbi, sizeof(mbi)))
  {
    return std::string();
  }

  wchar_t pathBuf[MAX_PATH];
  if (!GetModuleFileNameW(static_cast<HMODULE>(mbi.AllocationBase), pathBuf, MAX_PATH))
  {
    return std::string();
  }

  return vtksys::Encoding::ToNarrow(pathBuf);

#else
  Dl_info info;
  if (dladdr(ptr, &info) && info.dli_fname)
  {
    return std::string(info.dli_fname);
  }
  return std::string();
#endif
}

//------------------------------------------------------------------------------
VTK_FILEPATH std::string vtkResourceFileLocator::GetCurrentExecutablePath()
{
#if defined(_WIN32) && !defined(__CYGWIN__)
  wchar_t buf[MAX_PATH];
  DWORD size = GetModuleFileNameW(nullptr, buf, MAX_PATH);
  if (size == 0)
  {
    return {};
  }
  return vtksys::Encoding::ToNarrow(buf);

#elif defined(__linux__)
  char buf[PATH_MAX];
  ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
  if (len == -1)
  {
    return {};
  }
  buf[len] = '\0';
  return std::string(buf);

#elif defined(__APPLE__)
  char buf[PATH_MAX];
  uint32_t size = sizeof(buf);
  if (_NSGetExecutablePath(buf, &size) != 0)
  {
    return {};
  }
  return std::string(buf);

#else
  return std::string(); // fallback for unsupported platforms
#endif
}

//------------------------------------------------------------------------------
VTK_FILEPATH std::string vtkResourceFileLocator::GetLibraryPathForSymbolUnix(const char* symbolname)
{
#if !defined(_WIN32) || defined(__CYGWIN__)
  void* ptr = dlsym(RTLD_DEFAULT, symbolname);
  return vtkResourceFileLocator::GetLibraryPathForAddress(ptr);
#else
  (void)symbolname;
  return std::string();
#endif
}

//------------------------------------------------------------------------------
VTK_FILEPATH std::string vtkResourceFileLocator::GetLibraryPathForSymbolWin32(const void* fptr)
{
#if defined(_WIN32) && !defined(__CYGWIN__)
  return vtkResourceFileLocator::GetLibraryPathForAddress(fptr);
#else
  (void)fptr;
  return std::string();
#endif
}

//------------------------------------------------------------------------------
void vtkResourceFileLocator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "LogVerbosity: " << this->LogVerbosity << endl;
}
VTK_ABI_NAMESPACE_END
