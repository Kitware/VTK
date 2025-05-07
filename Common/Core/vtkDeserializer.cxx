// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDeserializer.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtksys/SystemInformation.hxx"

// clang-format off
#include "vtk_nlohmannjson.h"        // for json
#include VTK_NLOHMANN_JSON(json.hpp) // for json
// clang-format on

#include <exception>
#include <iterator>
#include <sstream>
#include <unordered_map>

VTK_ABI_NAMESPACE_BEGIN

class vtkDeserializer::vtkInternals
{
public:
  std::unordered_map<std::size_t, vtkDeserializer::HandlerType> Handlers;
  std::unordered_map<std::string, vtkDeserializer::ConstructorType> Constructors;
};

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkDeserializer);

//------------------------------------------------------------------------------
vtkDeserializer::vtkDeserializer()
  : Internals(new vtkInternals())
{
}

//------------------------------------------------------------------------------
vtkDeserializer::~vtkDeserializer() = default;

//------------------------------------------------------------------------------
void vtkDeserializer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  const auto& internals = (*this->Internals);
  os << "No. of handlers: " << internals.Handlers.size() << '\n';
  for (const auto& item : internals.Handlers)
  {
    os << item.first << ": function pointer (" << item.second.target_type().name() << ")\n";
  }
  os << "No. of constructors: " << internals.Constructors.size() << '\n';
  for (const auto& item : internals.Constructors)
  {
    os << item.first << ": function pointer (" << item.second.target_type().name() << ")\n";
  }
}

//------------------------------------------------------------------------------
vtkObjectBase* vtkDeserializer::ConstructObject(
  const std::string& className, const std::vector<std::string>& superClassNames)
{
  vtkObjectBase* objectBase = nullptr;
  if (const auto constructor = this->GetConstructor(className, superClassNames))
  {
    objectBase = constructor();
    vtkVLogF(this->GetDeserializerLogVerbosity(), "Constructing %s %s", className.c_str(),
      objectBase->GetObjectDescription().c_str());
  }
  if (objectBase == nullptr)
  {
    std::ostringstream scNames;
    std::copy(superClassNames.begin(), superClassNames.end() - 1,
      std::ostream_iterator<std::string>(scNames, ", "));
    scNames << superClassNames.back();
    vtkErrorMacro(<< "Constructor failed to create instance of " << className
                  << " with superClassNames : " << scNames.str());
  }
  return objectBase;
}

//------------------------------------------------------------------------------
bool vtkDeserializer::DeserializeJSON(
  const vtkTypeUInt32& identifier, vtkSmartPointer<vtkObjectBase>& objectBase)
{
  const auto& state = this->Context->GetState(identifier);
  if (state.empty())
  {
    return false;
  }
  std::string className;
  std::vector<std::string> superClassNames;
  if (objectBase == nullptr)
  {
    // Only look for ClassName and SuperClassNames if we are going to construct the object.
    {
      const auto iter = state.find("ClassName");
      if (iter == state.end())
      {
        vtkErrorMacro(<< "Failed to find 'ClassName' in state at id=" << identifier);
        return false;
      }
      else
      {
        className = iter->get<std::string>();
      }
    }
    {
      const auto iter = state.find("SuperClassNames");
      if (iter == state.end())
      {
        vtkErrorMacro(<< "Failed to find 'SuperClassNames' in state at id=" << identifier);
        return false;
      }
      else
      {
        superClassNames = iter->get<std::vector<std::string>>();
      }
    }
    if (auto ptr = this->ConstructObject(className, superClassNames))
    {
      objectBase = vtk::TakeSmartPointer(ptr);
      if (this->Context->GetObjectAtId(identifier) != objectBase)
      {
        this->Context->UnRegisterObject(identifier);
      }
      vtkTypeUInt32 registrationId = identifier;
      if (!this->Context->RegisterObject(ptr, registrationId))
      {
        vtkErrorMacro(<< "Failed to register " << ptr->GetObjectDescription() << " at "
                      << identifier);
        return false;
      }
    }
    else
    {
      return false;
    }
  }
  auto* objectPtr = objectBase.Get();
  if (this->Context->IsProcessed(identifier))
  {
    vtkVLogF(this->GetDeserializerLogVerbosity(), "Avoided deserialization of %s",
      objectBase->GetObjectDescription().c_str());
    this->Context->AddChild(identifier);
    return true;
  }
  else if (const auto& f = this->GetHandler(typeid(*objectPtr)))
  {
    if (this->Context->IsProcessing(identifier))
    {
      vtkVLogF(this->GetDeserializerLogVerbosity(), "Prevented recursive deserialization for %s",
        objectPtr->GetObjectDescription().c_str());
    }
    else
    {
      try
      {
        vtkMarshalContext::ScopedParentTracker parentTracker(this->Context, identifier);
        vtkVLogScopeF(this->GetDeserializerLogVerbosity(), "Deserialize %s at identifier=%u",
          objectPtr->GetObjectDescription().c_str(), identifier);
        f(state, objectPtr, this);
      }
      catch (std::exception& e)
      {
        vtkErrorMacro(<< "In \"" << __func__ << "\", failed to deserialize state=" << state.dump()
                      << ". message=" << e.what());
        return false;
      }
    }
    this->Context->AddChild(identifier);
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
void vtkDeserializer::RegisterConstructor(const std::string& className, ConstructorType constructor)
{
  auto& internals = (*this->Internals);
  vtkVLog(this->GetDeserializerLogVerbosity(), << "Register constructor for " << className);
  internals.Constructors[className] = constructor;
}

//------------------------------------------------------------------------------
vtkDeserializer::ConstructorType vtkDeserializer::GetConstructor(
  const std::string& className, const std::vector<std::string>& superClassNames)
{
  const auto& internals = (*this->Internals);
  std::vector<std::string> classNamesToTry = { className };
  // Note that the `superClassNames` is ordered from least derived to most derived.
  // For example, if the class hierarchy is A->B->C, the `superClassNames` will be ['A','B'] and
  // className will be 'C'. Since we are trying to construct C, we want to try C first, then B, and
  // finally A.
  // So we need to reverse the order of `superClassNames` to get correct order ['C','B','A']. This
  // is important for classes that use object factory to create the objects.
  classNamesToTry.insert(classNamesToTry.end(), superClassNames.rbegin(), superClassNames.rend());
  for (const auto& name : classNamesToTry)
  {
    auto iter = internals.Constructors.find(name);
    if (iter != internals.Constructors.end() && name != "vtkObject" && name != "vtkObjectBase")
    {
      return iter->second;
    }
  }
  vtkErrorMacro(<< "There is no constructor registered for type " << className
                << ". Check stack trace to see how we got here.");
  vtkWarningMacro(<< vtksys::SystemInformation::GetProgramStack(2, 1));
  return nullptr;
}

//------------------------------------------------------------------------------
void vtkDeserializer::UnRegisterConstructor(const std::string& className)
{
  auto& internals = (*this->Internals);
  internals.Constructors.erase(className);
}

//------------------------------------------------------------------------------
void vtkDeserializer::RegisterHandler(const std::type_info& type, HandlerType handler)
{
  auto& internals = (*this->Internals);
  vtkVLog(this->GetDeserializerLogVerbosity(),
    << "Register handler at { .name=" << type.name() << " .hashCode=" << type.hash_code() << " }");
  internals.Handlers[type.hash_code()] = handler;
}

//------------------------------------------------------------------------------
vtkDeserializer::HandlerType vtkDeserializer::GetHandler(const std::type_info& type) const
{
  const auto& internals = (*this->Internals);
  auto iter = internals.Handlers.find(type.hash_code());
  if (iter != internals.Handlers.end())
  {
    return iter->second;
  }
  vtkErrorMacro(
    << "Cannot deserialize object with type "
       "{ .name="
    << type.name() << " .hashCode=" << type.hash_code()
    << " }"
       " because a deserializer was not found. Check stack trace to see how we got here.");
  vtkWarningMacro(<< vtksys::SystemInformation::GetProgramStack(2, 1));
  return nullptr;
}

//------------------------------------------------------------------------------
bool vtkDeserializer::UnRegisterHandler(const std::type_info& type)
{
  return this->Internals->Handlers.erase(type.hash_code()) != 0;
}

//------------------------------------------------------------------------------
void vtkDeserializer::SetDeserializerLogVerbosity(vtkLogger::Verbosity verbosity)
{
  this->DeserializerLogVerbosity = verbosity;
}

//------------------------------------------------------------------------------
vtkLogger::Verbosity vtkDeserializer::GetDeserializerLogVerbosity()
{
  // initialize the log verbosity if it is invalid.
  if (this->DeserializerLogVerbosity == vtkLogger::VERBOSITY_INVALID)
  {
    this->DeserializerLogVerbosity = vtkLogger::VERBOSITY_TRACE;
    // Find an environment variable that specifies logger verbosity
    const char* verbosityKey = "VTK_DESERIALIZER_LOG_VERBOSITY";
    if (vtksys::SystemTools::HasEnv(verbosityKey))
    {
      const char* verbosityCStr = vtksys::SystemTools::GetEnv(verbosityKey);
      const auto verbosity = vtkLogger::ConvertToVerbosity(verbosityCStr);
      if (verbosity > vtkLogger::VERBOSITY_INVALID)
      {
        this->DeserializerLogVerbosity = verbosity;
      }
    }
  }
  return this->DeserializerLogVerbosity;
}
VTK_ABI_NAMESPACE_END
