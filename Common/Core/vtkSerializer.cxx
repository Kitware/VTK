// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSerializer.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtksys/SystemInformation.hxx"

// clang-format off
#include "vtk_nlohmannjson.h"
#include VTK_NLOHMANN_JSON(json.hpp)
// clang-format on

#include <exception>

VTK_ABI_NAMESPACE_BEGIN

class vtkSerializer::vtkInternals
{
public:
  std::unordered_map<std::size_t, vtkSerializer::HandlerType> Handlers;
};

vtkStandardNewMacro(vtkSerializer);

vtkSerializer::vtkSerializer()
  : Internals(new vtkInternals())
{
}

vtkSerializer::~vtkSerializer() = default;

void vtkSerializer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  const auto& internals = (*this->Internals);
  os << "No. of handlers: " << internals.Handlers.size() << '\n';
  for (const auto& item : internals.Handlers)
  {
    os << item.first << ": function pointer (" << item.second.target_type().name() << ")\n";
  }
}

nlohmann::json vtkSerializer::SerializeJSON(vtkObjectBase* objectBase)
{
  if ((this->Context == nullptr) || (objectBase == nullptr))
  {
    return nlohmann::json::object();
  }

  vtkTypeUInt32 identifier = 0;
  if (this->Context->HasId(objectBase, identifier))
  {
    if (this->Context->IsProcessing(identifier) || this->Context->IsProcessed(identifier))
    {
      vtkLogF(TRACE, "Avoided serialization of %s", objectBase->GetObjectDescription().c_str());
      this->Context->AddChild(identifier);
      return { { "Id", identifier } };
    }
  }
  else if (!this->Context->RegisterObject(objectBase, identifier))
  {
    vtkErrorMacro(<< "Failed to add object " << objectBase->GetObjectDescription());
    return nlohmann::json::object();
  }
  nlohmann::json state = nlohmann::json::object();
  if (const auto& f = this->GetHandler(typeid(*objectBase)))
  {
    try
    {
      vtkMarshalContext::ScopedParentTracker parentTracker(this->Context, identifier);
      vtkLogScopeF(TRACE, "Serialize objectBase=%s at id=%u",
        objectBase->GetObjectDescription().c_str(), identifier);
      state = f(objectBase, this);
      state["Id"] = identifier;
      this->Context->UnRegisterState(identifier);
    }
    catch (std::exception& e)
    {
      vtkErrorMacro(<< "Failed to serialize objectBase=" << objectBase->GetObjectDescription()
                    << ". message=" << e.what());
      return nlohmann::json::object();
    }
  }
  if (this->Context->RegisterState(std::move(state)))
  {
    this->Context->AddChild(identifier);
    return { { "Id", identifier } };
  }
  vtkErrorMacro(<< "Failed to add state for object=" << objectBase->GetObjectDescription()
                << " with id=" << identifier);
  return nlohmann::json::object();
}

void vtkSerializer::RegisterHandler(const std::type_info& type, HandlerType handler)
{
  auto& internals = (*this->Internals);
  vtkDebugMacro(<< "Register handler at { .name=" << type.name()
                << " .hashCode=" << type.hash_code() << " }");
  internals.Handlers[type.hash_code()] = handler;
}

vtkSerializer::HandlerType vtkSerializer::GetHandler(const std::type_info& type) const
{
  const auto& internals = (*this->Internals);
  auto handlerIter = internals.Handlers.find(type.hash_code());
  if (handlerIter != internals.Handlers.end())
  {
    return handlerIter->second;
  }
  vtkErrorMacro(
    << "Cannot serialize object with type "
       "{ .name="
    << type.name() << " .hashCode=" << type.hash_code()
    << " }"
       " because a serializer was not found. Check stack trace to see how we got here.");
  vtkWarningMacro(<< vtksys::SystemInformation::GetProgramStack(2, 1));
  return nullptr;
}

bool vtkSerializer::UnRegisterHandler(const std::type_info& type)
{
  return this->Internals->Handlers.erase(type.hash_code()) != 0;
}
VTK_ABI_NAMESPACE_END
