/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLogger.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLogger.h"

#include "vtkObjectFactory.h"

#if VTK_ENABLE_LOGGING
#include <vtk_loguru.h>
#endif

#include <memory>
#include <sstream>

//=============================================================================
class vtkLogger::LogScopeRAII::LSInternals
{
public:
#if VTK_ENABLE_LOGGING
  std::unique_ptr<loguru::LogScopeRAII> Data;
#endif
};

vtkLogger::LogScopeRAII::LogScopeRAII()
  : Internals(nullptr)
{
}

vtkLogger::LogScopeRAII::LogScopeRAII(vtkLogger::Verbosity verbosity,
  const char* fname,
  unsigned int lineno,
  const char* format,
  ...)
  : Internals(new LSInternals())
{
#if VTK_ENABLE_LOGGING
  va_list vlist;
  va_start(vlist, format);
  auto result = loguru::vstrprintf(format, vlist);
  va_end(vlist);
  this->Internals->Data.reset(new loguru::LogScopeRAII(
    static_cast<loguru::Verbosity>(verbosity), fname, lineno, "%s", result.c_str()));
#else
  (void)verbosity;
  (void)fname;
  (void)lineno;
  (void)format;
#endif
}

vtkLogger::LogScopeRAII::~LogScopeRAII()
{
  delete this->Internals;
}
//=============================================================================

vtkStandardNewMacro(vtkLogger);
//----------------------------------------------------------------------------
vtkLogger::vtkLogger() {}

//----------------------------------------------------------------------------
vtkLogger::~vtkLogger() {}

//----------------------------------------------------------------------------
void vtkLogger::Init(int& argc, char* argv[], const char* verbosity_flag /*= "-v"*/)
{
#if VTK_ENABLE_LOGGING
  loguru::g_preamble_date = false;
  loguru::g_preamble_time = false;
  loguru::init(argc, argv, verbosity_flag);
#else
  (void)argc;
  (void)argv;
  (void)verbosity_flag;
#endif
}

//----------------------------------------------------------------------------
void vtkLogger::SetStderrVerbosity(vtkLogger::Verbosity level)
{
#if VTK_ENABLE_LOGGING
  loguru::g_stderr_verbosity = static_cast<loguru::Verbosity>(level);
#else
  (void)level;
#endif
}

//----------------------------------------------------------------------------
void vtkLogger::LogToFile(const char* path,
  vtkLogger::FileMode filemode,
  vtkLogger::Verbosity verbosity)
{
#if VTK_ENABLE_LOGGING
  loguru::add_file(
    path, static_cast<loguru::FileMode>(filemode), static_cast<loguru::Verbosity>(verbosity));
#else
  (void)path;
  (void)filemode;
  (void)verbosity;
#endif
}

//----------------------------------------------------------------------------
void vtkLogger::EndLogToFile(const char* path)
{
#if VTK_ENABLE_LOGGING
  loguru::remove_callback(path);
#else
  (void)path;
#endif
}

//----------------------------------------------------------------------------
void vtkLogger::SetThreadName(const std::string& name)
{
#if VTK_ENABLE_LOGGING
  loguru::set_thread_name(name.c_str());
#else
  (void)name;
#endif
}

//----------------------------------------------------------------------------
std::string vtkLogger::GetThreadName()
{
#if VTK_ENABLE_LOGGING
  char buffer[128];
  loguru::get_thread_name(buffer, 128, false);
  return std::string(buffer);
#else
  return std::string("N/A");
#endif
}

//----------------------------------------------------------------------------
void vtkLogger::AddCallback(const char* id,
  vtkLogger::LogHandlerCallbackT callback,
  void* user_data,
  vtkLogger::Verbosity verbosity,
  vtkLogger::CloseHandlerCallbackT on_close,
  vtkLogger::FlushHandlerCallbackT on_flush)
{
#if VTK_ENABLE_LOGGING
  loguru::add_callback(id,
    reinterpret_cast<loguru::log_handler_t>(callback),
    user_data,
    static_cast<loguru::Verbosity>(verbosity),
    reinterpret_cast<loguru::close_handler_t>(on_close),
    reinterpret_cast<loguru::flush_handler_t>(on_flush));
#else
  (void)id;
  (void)callback;
  (void)user_data;
  (void)verbosity;
  (void)on_close;
  (void)on_flush;
#endif
}

//----------------------------------------------------------------------------
bool vtkLogger::RemoveCallback(const char* id)
{
#if VTK_ENABLE_LOGGING
  return loguru::remove_callback(id);
#else
  (void)id;
  return false;
#endif
}

//----------------------------------------------------------------------------
std::string vtkLogger::GetIdentifier(vtkObjectBase* obj)
{
  if (obj)
  {
    std::ostringstream str;
    str << obj->GetClassName() << " (" << obj << ")";
    return str.str();
  }
  return "(nullptr)";
}

//----------------------------------------------------------------------------
void vtkLogger::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObjectBase::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
bool vtkLogger::IsEnabled()
{
#if VTK_ENABLE_LOGGING
  return true;
#else
  return false;
#endif
}

//----------------------------------------------------------------------------
vtkLogger::Verbosity vtkLogger::GetCurrentVerbosityCutoff()
{
#if VTK_ENABLE_LOGGING
  return static_cast<vtkLogger::Verbosity>(loguru::current_verbosity_cutoff());
#else
  return VERBOSITY_INVALID;
#endif
}

//----------------------------------------------------------------------------
void vtkLogger::Log(vtkLogger::Verbosity verbosity,
  const char* fname,
  unsigned int lineno,
  const char* txt)
{
#if VTK_ENABLE_LOGGING
  loguru::log(static_cast<loguru::Verbosity>(verbosity), fname, lineno, "%s", txt);
#else
  (void)verbosity;
  (void)fname;
  (void)lineno;
  (void)txt;
#endif
}

//----------------------------------------------------------------------------
void vtkLogger::Logf(vtkLogger::Verbosity verbosity,
  const char* fname,
  unsigned int lineno,
  const char* format,
  ...)
{
#if VTK_ENABLE_LOGGING
  va_list vlist;
  va_start(vlist, format);
  auto result = loguru::vstrprintf(format, vlist);
  va_end(vlist);
  vtkLogger::Log(verbosity, fname, lineno, result.c_str());
#else
  (void)verbosity;
  (void)fname;
  (void)lineno;
  (void)format;
#endif
}
