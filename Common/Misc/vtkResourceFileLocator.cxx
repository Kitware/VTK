/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkResourceFileLocator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkResourceFileLocator.h"

#include "vtkLogger.h"
#include "vtkObjectFactory.h"

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

vtkStandardNewMacro(vtkResourceFileLocator);
//----------------------------------------------------------------------------
vtkResourceFileLocator::vtkResourceFileLocator()
  : LogVerbosity(vtkLogger::VERBOSITY_TRACE)
{
}

//----------------------------------------------------------------------------
vtkResourceFileLocator::~vtkResourceFileLocator() = default;

//----------------------------------------------------------------------------
std::string vtkResourceFileLocator::Locate(
  const std::string& anchor, const std::string& landmark, const std::string& defaultDir)
{
  return this->Locate(anchor, { std::string() }, landmark, defaultDir);
}

//----------------------------------------------------------------------------
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
      const std::string landmarkdir =
        curprefix.empty() ? curanchor : curanchor + VTK_PATH_SEPARATOR + curprefix;
      const std::string landmarktocheck = landmarkdir + VTK_PATH_SEPARATOR + landmark;
      if (vtksys::SystemTools::FileExists(landmarktocheck))
      {
        VTK_FILE_LOCATOR_DEBUG_MESSAGE("trying file %s -- success!", landmarktocheck.c_str());
        return landmarkdir;
      }
      else
      {
        VTK_FILE_LOCATOR_DEBUG_MESSAGE("trying file %s -- failed!", landmarktocheck.c_str());
      }
    }
    path_components.pop_back();
  }
  return defaultDir;
}

//----------------------------------------------------------------------------
std::string vtkResourceFileLocator::GetLibraryPathForSymbolUnix(const char* symbolname)
{
#if defined(_WIN32) && !defined(__CYGWIN__)
  (void)symbolname;
  return std::string();
#else
  void* handle = dlsym(RTLD_DEFAULT, symbolname);
  if (!handle)
  {
    return std::string();
  }

  Dl_info di;
  int ret = dladdr(handle, &di);
  if (ret == 0 || !di.dli_saddr || !di.dli_fname)
  {
    return std::string();
  }

  return std::string(di.dli_fname);
#endif
}

//----------------------------------------------------------------------------
std::string vtkResourceFileLocator::GetLibraryPathForSymbolWin32(const void* fptr)
{
#if defined(_WIN32) && !defined(__CYGWIN__)
  MEMORY_BASIC_INFORMATION mbi;
  VirtualQuery(fptr, &mbi, sizeof(mbi));
  char pathBuf[16384];
  if (!GetModuleFileName(static_cast<HMODULE>(mbi.AllocationBase), pathBuf, sizeof(pathBuf)))
  {
    return std::string();
  }

  return std::string(pathBuf);
#else
  (void)fptr;
  return std::string();
#endif
}

//----------------------------------------------------------------------------
void vtkResourceFileLocator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "LogVerbosity: " << this->LogVerbosity << endl;
}

#if !defined(VTK_LEGACY_REMOVE)
void vtkResourceFileLocator::SetPrintDebugInformation(bool val)
{
  VTK_LEGACY_REPLACED_BODY(vtkResourceFileLocator::SetPrintDebugInformation, "VTK 8.3",
    vtkResourceFileLocator::SetLogVerbosity);
  this->SetLogVerbosity(val ? vtkLogger::VERBOSITY_INFO : vtkLogger::VERBOSITY_TRACE);
}

bool vtkResourceFileLocator::GetPrintDebugInformation()
{
  VTK_LEGACY_REPLACED_BODY(vtkResourceFileLocator::GetPrintDebugInformation, "VTK 8.3",
    vtkResourceFileLocator::GetLogVerbosity);
  return (this->GetLogVerbosity() == vtkLogger::VERBOSITY_INFO);
}

void vtkResourceFileLocator::PrintDebugInformationOn()
{
  VTK_LEGACY_REPLACED_BODY(vtkResourceFileLocator::PrintDebugInformationOn, "VTK 8.3",
    vtkResourceFileLocator::SetLogVerbosity);
  this->SetLogVerbosity(vtkLogger::VERBOSITY_INFO);
}

void vtkResourceFileLocator::PrintDebugInformationOff()
{
  VTK_LEGACY_REPLACED_BODY(vtkResourceFileLocator::PrintDebugInformationOff, "VTK 8.3",
    vtkResourceFileLocator::SetLogVerbosity);
  this->SetLogVerbosity(vtkLogger::VERBOSITY_TRACE);
}

#endif
