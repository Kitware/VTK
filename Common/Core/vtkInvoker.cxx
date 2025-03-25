// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkInvoker.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"

#include "vtksys/SystemTools.hxx"

// clang-format off
#include "vtk_nlohmannjson.h"        // for json
#include VTK_NLOHMANN_JSON(json.hpp) // for json
// clang-format on

#include <unordered_map>

VTK_ABI_NAMESPACE_BEGIN

class vtkInvoker::vtkInternals
{
public:
  std::unordered_map<std::size_t, vtkInvoker::HandlerType> Handlers;
};

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkInvoker);

//------------------------------------------------------------------------------
vtkInvoker::vtkInvoker()
  : Internals(new vtkInternals())
{
}

//------------------------------------------------------------------------------
vtkInvoker::~vtkInvoker() = default;

//------------------------------------------------------------------------------
void vtkInvoker::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  const auto& internals = (*this->Internals);
  os << "No. of handlers: " << internals.Handlers.size() << '\n';
  for (const auto& item : internals.Handlers)
  {
    os << item.first << ": function pointer (" << item.second.target_type().name() << ")\n";
  }
}

//------------------------------------------------------------------------------
nlohmann::json vtkInvoker::Invoke(
  const vtkTypeUInt32& identifier, const std::string& methodName, const nlohmann::json& args)
{
  if (auto objectBase = this->Context->GetObjectAtId(identifier))
  {
    auto* objectPtr = objectBase.GetPointer();
    const auto& typeId = typeid(*objectPtr);
    if (const auto& f = this->GetHandler(typeId))
    {
      vtkVLog(this->GetInvokerLogVerbosity(), << "Invoke method=\'" << methodName.c_str()
                                              << "\', args=\'" << args.dump().c_str() << "\'");
      return f(this, objectPtr, methodName.c_str(), args);
    }
    else
    {
      vtkErrorMacro(
        << "Cannot invoke method on object with type "
           "{ .name="
        << typeId.name() << " .hashCode=" << typeId.hash_code()
        << " }"
           " because a handler was not found. Check stack trace to see how we got here.");
    }
  }
  else
  {
    vtkWarningMacro(<< "Cannot invoke method \'" << methodName.c_str()
                    << "\' on an object (id=" << identifier << ") that does not exist");
  }
  return { { "success", false } };
}

//------------------------------------------------------------------------------
void vtkInvoker::RegisterHandler(const std::type_info& type, HandlerType invoker)
{
  auto& internals = (*this->Internals);
  vtkVLog(this->GetInvokerLogVerbosity(),
    << "Register invoker at { .name=" << type.name() << " .hashCode=" << type.hash_code() << " }");
  internals.Handlers[type.hash_code()] = invoker;
}

//------------------------------------------------------------------------------
vtkInvoker::HandlerType vtkInvoker::GetHandler(const std::type_info& type) const
{
  const auto& internals = (*this->Internals);
  auto iter = internals.Handlers.find(type.hash_code());
  if (iter != internals.Handlers.end())
  {
    return iter->second;
  }
  return nullptr;
}

//------------------------------------------------------------------------------
bool vtkInvoker::UnRegisterHandler(const std::type_info& type)
{
  return this->Internals->Handlers.erase(type.hash_code()) != 0;
}

//------------------------------------------------------------------------------
void vtkInvoker::SetInvokerLogVerbosity(vtkLogger::Verbosity verbosity)
{
  this->InvokerLogVerbosity = verbosity;
}

//------------------------------------------------------------------------------
vtkLogger::Verbosity vtkInvoker::GetInvokerLogVerbosity()
{
  // initialize the log verbosity if it is invalid.
  if (this->InvokerLogVerbosity == vtkLogger::VERBOSITY_INVALID)
  {
    this->InvokerLogVerbosity = vtkLogger::VERBOSITY_TRACE;
    // Find an environment variable that specifies logger verbosity
    const char* verbosityKey = "VTK_INVOKER_LOG_VERBOSITY";
    if (vtksys::SystemTools::HasEnv(verbosityKey))
    {
      const char* verbosityCStr = vtksys::SystemTools::GetEnv(verbosityKey);
      const auto verbosity = vtkLogger::ConvertToVerbosity(verbosityCStr);
      if (verbosity > vtkLogger::VERBOSITY_INVALID)
      {
        this->InvokerLogVerbosity = verbosity;
      }
    }
  }
  return this->InvokerLogVerbosity;
}
VTK_ABI_NAMESPACE_END
