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

#if VTK_MODULE_ENABLE_VTK_loguru
#include <vtk_loguru.h>
#endif

#include <cstdlib>
#include <memory>
#include <mutex>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <vector>

//=============================================================================
class vtkLogger::LogScopeRAII::LSInternals
{
public:
#if VTK_MODULE_ENABLE_VTK_loguru
  std::unique_ptr<loguru::LogScopeRAII> Data;
#endif
};

vtkLogger::LogScopeRAII::LogScopeRAII()
  : Internals(nullptr)
{
}

vtkLogger::LogScopeRAII::LogScopeRAII(
  vtkLogger::Verbosity verbosity, const char* fname, unsigned int lineno, const char* format, ...)
#if VTK_MODULE_ENABLE_VTK_loguru
  : Internals(new LSInternals())
#else
  : Internals(nullptr)
#endif
{
#if VTK_MODULE_ENABLE_VTK_loguru
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

namespace detail
{
#if VTK_MODULE_ENABLE_VTK_loguru
using scope_pair = std::pair<std::string, std::shared_ptr<loguru::LogScopeRAII> >;
static std::mutex g_mutex;
static std::unordered_map<std::thread::id, std::vector<scope_pair> > g_vectors;
static std::vector<scope_pair>& get_vector()
{
  std::lock_guard<std::mutex> guard(g_mutex);
  return g_vectors[std::this_thread::get_id()];
}

static void push_scope(const char* id, std::shared_ptr<loguru::LogScopeRAII> ptr)
{
  get_vector().push_back(std::make_pair(std::string(id), ptr));
}

static void pop_scope(const char* id)
{
  auto& vector = get_vector();
  if (vector.size() > 0 && vector.back().first == id)
  {
    vector.pop_back();

    if (vector.empty())
    {
      std::lock_guard<std::mutex> guard(g_mutex);
      g_vectors.erase(std::this_thread::get_id());
    }
  }
  else
  {
    LOG_F(ERROR, "Mismatched scope! expected (%s), got (%s)", vector.back().first.c_str(), id);
  }
}
#endif
}

//=============================================================================
//----------------------------------------------------------------------------
vtkLogger::vtkLogger() {}

//----------------------------------------------------------------------------
vtkLogger::~vtkLogger() {}

//----------------------------------------------------------------------------
void vtkLogger::Init(int& argc, char* argv[], const char* verbosity_flag /*= "-v"*/)
{
#if VTK_MODULE_ENABLE_VTK_loguru
  if (argc == 0)
  { // loguru::init can't handle this case -- call the no-arg overload.
    vtkLogger::Init();
    return;
  }

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
void vtkLogger::Init()
{
  int argc = 1;
  char dummy[1] = { '\0' };
  char* argv[2] = { dummy, nullptr };
  vtkLogger::Init(argc, argv);
}

//----------------------------------------------------------------------------
void vtkLogger::SetStderrVerbosity(vtkLogger::Verbosity level)
{
#if VTK_MODULE_ENABLE_VTK_loguru
  loguru::g_stderr_verbosity = static_cast<loguru::Verbosity>(level);
#else
  (void)level;
#endif
}

//----------------------------------------------------------------------------
void vtkLogger::LogToFile(
  const char* path, vtkLogger::FileMode filemode, vtkLogger::Verbosity verbosity)
{
#if VTK_MODULE_ENABLE_VTK_loguru
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
#if VTK_MODULE_ENABLE_VTK_loguru
  loguru::remove_callback(path);
#else
  (void)path;
#endif
}

//----------------------------------------------------------------------------
void vtkLogger::SetThreadName(const std::string& name)
{
#if VTK_MODULE_ENABLE_VTK_loguru
  loguru::set_thread_name(name.c_str());
#else
  (void)name;
#endif
}

//----------------------------------------------------------------------------
std::string vtkLogger::GetThreadName()
{
#if VTK_MODULE_ENABLE_VTK_loguru
  char buffer[128];
  loguru::get_thread_name(buffer, 128, false);
  return std::string(buffer);
#else
  return std::string("N/A");
#endif
}

//----------------------------------------------------------------------------
void vtkLogger::AddCallback(const char* id, vtkLogger::LogHandlerCallbackT callback,
  void* user_data, vtkLogger::Verbosity verbosity, vtkLogger::CloseHandlerCallbackT on_close,
  vtkLogger::FlushHandlerCallbackT on_flush)
{
#if VTK_MODULE_ENABLE_VTK_loguru
  loguru::add_callback(id, reinterpret_cast<loguru::log_handler_t>(callback), user_data,
    static_cast<loguru::Verbosity>(verbosity), reinterpret_cast<loguru::close_handler_t>(on_close),
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
#if VTK_MODULE_ENABLE_VTK_loguru
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
#if VTK_MODULE_ENABLE_VTK_loguru
  return true;
#else
  return false;
#endif
}

//----------------------------------------------------------------------------
vtkLogger::Verbosity vtkLogger::GetCurrentVerbosityCutoff()
{
#if VTK_MODULE_ENABLE_VTK_loguru
  return static_cast<vtkLogger::Verbosity>(loguru::current_verbosity_cutoff());
#else
  return VERBOSITY_INVALID; // return lowest value so no logging macros will be evaluated.
#endif
}

//----------------------------------------------------------------------------
void vtkLogger::Log(
  vtkLogger::Verbosity verbosity, const char* fname, unsigned int lineno, const char* txt)
{
#if VTK_MODULE_ENABLE_VTK_loguru
  loguru::log(static_cast<loguru::Verbosity>(verbosity), fname, lineno, "%s", txt);
#else
  (void)verbosity;
  (void)fname;
  (void)lineno;
  (void)txt;
#endif
}

//----------------------------------------------------------------------------
void vtkLogger::LogF(
  vtkLogger::Verbosity verbosity, const char* fname, unsigned int lineno, const char* format, ...)
{
#if VTK_MODULE_ENABLE_VTK_loguru
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

//----------------------------------------------------------------------------
void vtkLogger::StartScope(
  Verbosity verbosity, const char* id, const char* fname, unsigned int lineno)
{
#if VTK_MODULE_ENABLE_VTK_loguru
  detail::push_scope(id,
    verbosity > vtkLogger::GetCurrentVerbosityCutoff()
      ? std::make_shared<loguru::LogScopeRAII>()
      : std::make_shared<loguru::LogScopeRAII>(
          static_cast<loguru::Verbosity>(verbosity), fname, lineno, "%s", id));
#else
  (void)verbosity;
  (void)id;
  (void)fname;
  (void)lineno;
#endif
}

//----------------------------------------------------------------------------
void vtkLogger::EndScope(const char* id)
{
#if VTK_MODULE_ENABLE_VTK_loguru
  detail::pop_scope(id);
#else
  (void)id;
#endif
}

//----------------------------------------------------------------------------
void vtkLogger::StartScopeF(Verbosity verbosity, const char* id, const char* fname,
  unsigned int lineno, const char* format, ...)
{
#if VTK_MODULE_ENABLE_VTK_loguru
  if (verbosity > vtkLogger::GetCurrentVerbosityCutoff())
  {
    detail::push_scope(id, std::make_shared<loguru::LogScopeRAII>());
  }
  else
  {
    va_list vlist;
    va_start(vlist, format);
    auto result = loguru::vstrprintf(format, vlist);
    va_end(vlist);

    detail::push_scope(id,
      std::make_shared<loguru::LogScopeRAII>(
        static_cast<loguru::Verbosity>(verbosity), fname, lineno, "%s", result.c_str()));
  }
#else
  (void)verbosity;
  (void)id;
  (void)fname;
  (void)lineno;
  (void)format;
#endif
}

//----------------------------------------------------------------------------
vtkLogger::Verbosity vtkLogger::ConvertToVerbosity(int value)
{
  if (value <= vtkLogger::VERBOSITY_INVALID)
  {
    return vtkLogger::VERBOSITY_INVALID;
  }
  else if (value > vtkLogger::VERBOSITY_MAX)
  {
    return vtkLogger::VERBOSITY_MAX;
  }
  return static_cast<vtkLogger::Verbosity>(value);
}

//----------------------------------------------------------------------------
vtkLogger::Verbosity vtkLogger::ConvertToVerbosity(const char* text)
{
  if (text != nullptr)
  {
    char* end = nullptr;
    const int ivalue = static_cast<int>(std::strtol(text, &end, 10));
    if (end != text && *end == '\0')
    {
      return vtkLogger::ConvertToVerbosity(ivalue);
    }
    if (std::string("OFF").compare(text) == 0)
    {
      return vtkLogger::VERBOSITY_OFF;
    }
    else if (std::string("ERROR").compare(text) == 0)
    {
      return vtkLogger::VERBOSITY_ERROR;
    }
    else if (std::string("WARNING").compare(text) == 0)
    {
      return vtkLogger::VERBOSITY_WARNING;
    }
    else if (std::string("INFO").compare(text) == 0)
    {
      return vtkLogger::VERBOSITY_INFO;
    }
    else if (std::string("TRACE").compare(text) == 0)
    {
      return vtkLogger::VERBOSITY_TRACE;
    }
    else if (std::string("MAX").compare(text) == 0)
    {
      return vtkLogger::VERBOSITY_MAX;
    }
  }
  return vtkLogger::VERBOSITY_INVALID;
}
