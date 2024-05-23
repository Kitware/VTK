// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkMarshalContext.h"

#include "vtkDataArrayRange.h"
#include "vtkObjectBase.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

// clang-format off
#include "vtk_nlohmannjson.h"
#include VTK_NLOHMANN_JSON(json.hpp)
// clang-format on

#include <stack>
#include <unordered_set>
#include <utility>

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkMarshalContext);

//------------------------------------------------------------------------------
class vtkMarshalContext::vtkInternals
{
public:
  // UniqueId for each registered vtk object.
  vtkTypeUInt32 UniqueId = 0;
  // The global state of objects that serializers write into or deserializers read from.
  nlohmann::json States = nlohmann::json::object();
  // Cache for data arrays.
  nlohmann::json Blobs = nlohmann::json::object();
  // Placeholder returned by reference when an identifier doesn't have a state.
  nlohmann::json Empty = nlohmann::json::object();
  // The global store of weak references to objects.
  vtkMarshalContext::WeakObjectStore WeakObjects;
  // Object manager or deserializer will want to keep strong references to objects
  // that were registered through object manager or deserialized with the strong-ref attribute.
  vtkMarshalContext::StrongObjectStore StrongObjects;
  // Prevents recursion when dealing with circular dependencies and records hierarchy.
  std::unordered_set<vtkTypeUInt32> Visited;
  std::stack<vtkTypeUInt32> IdentifierStack;
  std::unordered_map<vtkTypeUInt32, std::set<vtkTypeUInt32>> Tree;
  // ephemeral storage of children in the current parent.
  // These will be added inside Tree in PopParent
  std::unordered_map<vtkTypeUInt32, std::set<vtkTypeUInt32>> CurrentTree;
};

//------------------------------------------------------------------------------
vtkMarshalContext::vtkMarshalContext()
  : Internals(new vtkInternals())
{
}

//------------------------------------------------------------------------------
vtkMarshalContext::~vtkMarshalContext() = default;

//------------------------------------------------------------------------------
void vtkMarshalContext::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << "No. of states: " << this->States().size() << '\n';
  os << "States: \n";
  os << indent << this->States().dump() << '\n';

  os << "No. of blobs: " << this->Blobs().size() << '\n';
  os << "Blobs: \n";
  os << indent << this->Blobs().dump() << '\n';

  os << "No. of weak objects: " << this->WeakObjects().size() << '\n';
  os << "WeakObjects: \n";
  for (const auto& iter : this->WeakObjects())
  {
    os << indent << iter.first << ": "
       << (iter.second ? iter.second->GetObjectDescription() : "[gone] nullptr") << '\n';
    if (iter.second)
    {
      iter.second->PrintSelf(os, indent.GetNextIndent());
    }
  }

  os << "No. of strong object records: " << this->StrongObjects().size() << '\n';
  os << "StrongObjects: \n";
  for (const auto& objectSet : this->StrongObjects())
  {
    os << "Owner: " << objectSet.first << '\n';
    for (const auto& object : objectSet.second)
    {
      object->PrintHeader(os, indent.GetNextIndent());
      object->PrintSelf(os, indent.GetNextIndent());
      object->PrintTrailer(os, indent.GetNextIndent());
    }
  }
}

//------------------------------------------------------------------------------
const nlohmann::json& vtkMarshalContext::Blobs() const
{
  return this->Internals->Blobs;
}

//------------------------------------------------------------------------------
const nlohmann::json& vtkMarshalContext::States() const
{
  return this->Internals->States;
}

//------------------------------------------------------------------------------
const vtkMarshalContext::WeakObjectStore& vtkMarshalContext::WeakObjects() const
{
  return this->Internals->WeakObjects;
}

//------------------------------------------------------------------------------
const vtkMarshalContext::StrongObjectStore& vtkMarshalContext::StrongObjects() const
{
  return this->Internals->StrongObjects;
}

//------------------------------------------------------------------------------
void vtkMarshalContext::KeepAlive(const std::string& owner, vtkObjectBase* objectBase)
{
  this->Internals->StrongObjects[owner].insert(objectBase);
}

//------------------------------------------------------------------------------
void vtkMarshalContext::Retire(const std::string& owner, vtkObjectBase* objectBase)
{
  this->Internals->StrongObjects[owner].erase(objectBase);
}

//------------------------------------------------------------------------------
bool vtkMarshalContext::RegisterState(nlohmann::json state)
{
  auto& internals = (*this->Internals);
  auto idIter = state.find("Id");
  if ((idIter != state.end()) && idIter->is_number_unsigned())
  {
    const auto identifier = idIter->get<vtkTypeUInt32>();
    const auto key = std::to_string(identifier);
    auto stateIter = internals.States.find(key);
    if (stateIter == internals.States.end())
    {
      return internals.States.emplace(std::to_string(identifier), std::move(state)).second;
    }
    else
    {
      stateIter->swap(state);
      return true;
    }
  }
  return false;
}

//------------------------------------------------------------------------------
bool vtkMarshalContext::UnRegisterState(vtkTypeUInt32 identifier)
{
  return this->Internals->States.erase(std::to_string(identifier)) == 1;
}

//------------------------------------------------------------------------------
nlohmann::json& vtkMarshalContext::GetState(vtkTypeUInt32 identifier) const
{
  auto& internals = (*this->Internals);
  auto stateIter = internals.States.find(std::to_string(identifier));
  if (stateIter != internals.States.end())
  {
    return stateIter.value();
  }
  else
  {
    return internals.Empty;
  }
}

//------------------------------------------------------------------------------
bool vtkMarshalContext::RegisterObject(vtkObjectBase* objectBase, vtkTypeUInt32& identifier)
{
  auto& internals = (*this->Internals);
  if (objectBase == nullptr)
  {
    return false;
  }
  if (identifier == 0)
  {
    identifier = this->MakeId();
  }
  return internals.WeakObjects.emplace(identifier, objectBase).second;
}

//------------------------------------------------------------------------------
bool vtkMarshalContext::UnRegisterObject(vtkTypeUInt32 identifier)
{
  return this->Internals->WeakObjects.erase(identifier) == 1;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkObjectBase> vtkMarshalContext::GetObjectAtId(vtkTypeUInt32 identifier) const
{
  auto& internals = (*this->Internals);
  auto objectIter = internals.WeakObjects.find(identifier);
  if (objectIter != internals.WeakObjects.end())
  {
    return vtk::MakeSmartPointer(objectIter->second.Get());
  }
  return nullptr;
}

//------------------------------------------------------------------------------
vtkTypeUInt32 vtkMarshalContext::GetId(vtkObjectBase* objectBase) const
{
  auto& internals = (*this->Internals);
  auto objectIter = std::find_if(internals.WeakObjects.begin(), internals.WeakObjects.end(),
    [objectBase](const std::pair<const vtkTypeUInt32, vtkWeakPointer<vtkObjectBase>>& item) {
      return item.second == objectBase;
    });
  if (objectIter != internals.WeakObjects.end())
  {
    return objectIter->first;
  }
  return 0;
}

//------------------------------------------------------------------------------
bool vtkMarshalContext::RegisterBlob(vtkSmartPointer<vtkTypeUInt8Array> blob, std::string& hash)
{
  auto& internals = (*this->Internals);
  if (blob == nullptr)
  {
    return false;
  }
  using namespace nlohmann;
  const auto& blobRange = vtk::DataArrayValueRange(blob.Get());
  auto binaryContainer =
    json::binary(std::vector<json::binary_t::value_type>(blobRange.begin(), blobRange.end()));
  if (hash.empty())
  {
    hash = std::to_string(std::hash<json>{}(binaryContainer));
  }
  if (internals.Blobs.contains(hash))
  {
    // return true to indicate blob already exists.
    return true;
  }
  return internals.Blobs.emplace(hash, std::move(binaryContainer)).second;
}

//------------------------------------------------------------------------------
bool vtkMarshalContext::UnRegisterBlob(const std::string& hash)
{
  return this->Internals->Blobs.erase(hash) == 1;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkTypeUInt8Array> vtkMarshalContext::GetBlob(const std::string& hash)
{
  auto& internals = (*this->Internals);
  vtkSmartPointer<vtkTypeUInt8Array> result;

  const auto blobIter = this->Internals->Blobs.find(hash);
  if (blobIter != internals.Blobs.end())
  {
    const auto& values = blobIter->get_binary();
    if (!values.empty())
    {
      result.TakeReference(vtkTypeUInt8Array::New());
      result->SetNumberOfValues(values.size());
      auto blobRange = vtk::DataArrayValueRange(result);
      std::copy(values.begin(), values.end(), blobRange.begin());
    }
  }
  return result;
}

//------------------------------------------------------------------------------
std::vector<vtkTypeUInt32> vtkMarshalContext::GetDirectDependencies(vtkTypeUInt32 identifier) const
{
  auto& internals = (*this->Internals);
  auto iter = internals.Tree.find(identifier);
  if (iter != internals.Tree.end())
  {
    return std::vector<vtkTypeUInt32>(iter->second.begin(), iter->second.end());
  }
  else
  {
    return {};
  }
}

//------------------------------------------------------------------------------
void vtkMarshalContext::ResetDirectDependencies()
{
  this->Internals->Tree.clear();
}

//------------------------------------------------------------------------------
vtkTypeUInt32 vtkMarshalContext::MakeId()
{
  return (++(this->Internals->UniqueId));
}

//------------------------------------------------------------------------------
void vtkMarshalContext::PushParent(vtkTypeUInt32 identifier)
{
  auto& internals = (*this->Internals);
  internals.Visited.insert(identifier);
  internals.IdentifierStack.push(identifier);
  internals.CurrentTree[identifier].clear();
}

//------------------------------------------------------------------------------
void vtkMarshalContext::PopParent()
{
  auto& internals = (*this->Internals);
  assert(!internals.IdentifierStack.empty());
  const auto parent = internals.IdentifierStack.top();
  const auto childrenIter = internals.CurrentTree.find(parent);
  if (childrenIter != internals.CurrentTree.end())
  {
    if (!childrenIter->second.empty())
    {
      internals.Tree.emplace(parent, childrenIter->second);
    }
  }
  internals.Visited.erase(parent);
  internals.IdentifierStack.pop();
}

//------------------------------------------------------------------------------
// vtkSerializer/vtkDeserializer common API.
//------------------------------------------------------------------------------
bool vtkMarshalContext::HasId(vtkObjectBase* objectBase, vtkTypeUInt32& identifier)
{
  identifier = this->GetId(objectBase);
  return identifier != 0;
}

//------------------------------------------------------------------------------
bool vtkMarshalContext::IsProcessing(vtkTypeUInt32 identifier)
{
  auto& internals = (*this->Internals);
  return internals.Visited.count(identifier);
}

//------------------------------------------------------------------------------
bool vtkMarshalContext::IsProcessed(vtkTypeUInt32 identifier)
{
  auto& internals = (*this->Internals);
  if (this->Internals->Tree.count(identifier) == 1)
  {
    return true;
  }
  if (!internals.IdentifierStack.empty())
  {
    const auto parent = internals.IdentifierStack.top();
    return internals.CurrentTree[parent].count(identifier) == 1;
  }
  return false;
}

//------------------------------------------------------------------------------
void vtkMarshalContext::AddChild(vtkTypeUInt32 identifier)
{
  auto& internals = (*this->Internals);
  if (internals.IdentifierStack.empty())
  {
    return;
  }
  const auto parent = internals.IdentifierStack.top();
  internals.CurrentTree[parent].insert(identifier);
}

VTK_ABI_NAMESPACE_END
