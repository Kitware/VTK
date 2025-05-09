// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkObjectManager.h"

#include "vtkDataArrayRange.h"
#include "vtkDataObject.h"
#include "vtkDeserializer.h"
#include "vtkMarshalContext.h"
#include "vtkObjectFactory.h"
#include "vtkSerializer.h"
#include "vtkTypeUInt8Array.h"

// clang-format off
#include "vtk_nlohmannjson.h"
#include VTK_NLOHMANN_JSON(json.hpp)
// clang-format on

#include <deque>
#include <fstream>
#include <unordered_set>

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
extern "C"
{
  int RegisterLibraries_vtkObjectManagerDefaultSerDes(
    void* ser, void* deser, void* invoker, const char** error);
}

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkObjectManager);

//------------------------------------------------------------------------------
vtkObjectManager::vtkObjectManager()
{
  this->Context = vtk::TakeSmartPointer(vtkMarshalContext::New());
  this->Deserializer->SetContext(this->Context);
  this->Serializer->SetContext(this->Context);
  this->Invoker->SetContext(this->Context);
}

//------------------------------------------------------------------------------
vtkObjectManager::~vtkObjectManager() = default;

//------------------------------------------------------------------------------
void vtkObjectManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << "Memory usage of blobs: " << this->GetTotalBlobMemoryUsage() << " bytes\n";
  os << "Memory usage of data objects: " << this->GetTotalVTKDataObjectMemoryUsage() << " bytes\n";

  os << "Context: \n";
  if (this->Context)
  {
    this->Context->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(null)\n";
  }

  os << "Deserializer:\n";
  this->Deserializer->PrintSelf(os, indent.GetNextIndent());

  os << "Serializer:\n";
  this->Serializer->PrintSelf(os, indent.GetNextIndent());
}

//------------------------------------------------------------------------------
std::size_t vtkObjectManager::GetTotalBlobMemoryUsage()
{
  std::size_t result = 0;
  for (const auto& iter : this->Context->Blobs().items())
  {
    auto values = iter.value().get_ptr<const nlohmann::json::binary_t*>();
    result += values->size();
  }
  return result;
}

//------------------------------------------------------------------------------
std::size_t vtkObjectManager::GetTotalVTKDataObjectMemoryUsage()
{
  std::size_t result = 0;
  for (const auto& iter : this->Context->WeakObjects())
  {
    if (auto dobj = vtkDataObject::SafeDownCast(iter.second))
    {
      result += dobj->GetActualMemorySize() * 1000;
    }
  }
  return result;
}

//------------------------------------------------------------------------------
bool vtkObjectManager::Initialize()
{
  if (!this->InitializeDefaultHandlers())
  {
    vtkErrorMacro(<< "Failed to register default VTK SerDes handlers! Some objects may not get "
                     "(de)serialized.");
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkObjectManager::InitializeDefaultHandlers()
{
  const char* error = nullptr;
  if (!RegisterLibraries_vtkObjectManagerDefaultSerDes(
        this->Serializer.Get(), this->Deserializer.Get(), this->Invoker.Get(), &error))
  {
    vtkErrorMacro(<< "Failed to register a default VTK SerDes handler. error=\"" << error << "\"");
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkObjectManager::InitializeExtensionModuleHandlers(
  const std::vector<RegistrarType>& registrars)
{
  for (const auto& registrar : registrars)
  {
    const char* error = nullptr;
    if (!registrar(this->Serializer, this->Deserializer, this->Invoker, &error))
    {
      vtkErrorMacro(<< "Failed to register an extension SerDes handler. error=\"" << error << "\"");
      return false;
    }
  }
  return true;
}

//------------------------------------------------------------------------------
void vtkObjectManager::Export(const std::string& filename, int indent, char indentChar)
{
  std::string objectStatesFileName = filename, blobsFileName = filename + ".blobs.json";
  if (!vtksys::SystemTools::StringEndsWith(filename, ".json"))
  {
    objectStatesFileName = filename + ".states.json";
    blobsFileName = filename + ".blobs.json";
  }
  {
    std::ofstream ofs(objectStatesFileName);
    try
    {
      ofs << this->Context->States().dump(indent, indentChar);
    }
    catch (nlohmann::json::type_error& e)
    {
      vtkErrorMacro(<< "Failed to dump json. message=" << e.what());
    }
  }
  {
    std::ofstream ofs(blobsFileName);
    try
    {
      ofs << this->Context->Blobs().dump(indent, indentChar);
    }
    catch (nlohmann::json::type_error& e)
    {
      vtkErrorMacro(<< "Failed to dump json. message=" << e.what());
    }
  }
}

//------------------------------------------------------------------------------
void vtkObjectManager::Import(const std::string& stateFileName, const std::string& blobFileName)
{
  this->Clear();
  // Register all the states.
  try
  {
    nlohmann::json states = nlohmann::json::parse(std::ifstream(stateFileName));
    for (const auto& state : states.items())
    {
      this->Context->RegisterState(state.value());
    }
  }
  catch (nlohmann::json::type_error& e)
  {
    vtkErrorMacro(<< "Failed to parse states from " << stateFileName << ". message=" << e.what());
  }
  // Register all the blobs.
  try
  {
    nlohmann::json blobs = nlohmann::json::parse(std::ifstream(blobFileName));
    for (const auto& blob : blobs.items())
    {
      auto hash = blob.key();
      const auto& values = blob.value().at("bytes").get<std::vector<vtkTypeUInt8>>();
      if (!values.empty())
      {
        auto byteArray = vtk::TakeSmartPointer(vtkTypeUInt8Array::New());
        byteArray->SetNumberOfValues(values.size());
        auto blobRange = vtk::DataArrayValueRange(byteArray);
        std::copy(values.begin(), values.end(), blobRange.begin());
        this->Context->RegisterBlob(byteArray, hash);
      }
    }
  }
  catch (nlohmann::json::type_error& e)
  {
    vtkErrorMacro(<< "Failed to parse blobs from " << blobFileName << ". message=" << e.what());
  }
  // Creates objects and deserializes states.
  this->UpdateObjectsFromStates();
}

//------------------------------------------------------------------------------
vtkTypeUInt32 vtkObjectManager::RegisterObject(vtkSmartPointer<vtkObjectBase> objectBase)
{
  if (objectBase == nullptr)
  {
    return 0;
  }
  this->Context->KeepAlive(this->OWNERSHIP_KEY(), objectBase);
  vtkTypeUInt32 identifier = 0;
  this->Context->RegisterObject(objectBase, identifier);
  return identifier;
}

//------------------------------------------------------------------------------
bool vtkObjectManager::UnRegisterObject(vtkTypeUInt32 identifier)
{
  if (auto object = this->Context->GetObjectAtId(identifier))
  {
    this->Context->Retire(this->OWNERSHIP_KEY(), object);
    this->Context->Retire(this->Deserializer->GetObjectDescription(), object);
  }
  return this->Context->UnRegisterObject(identifier);
}

//------------------------------------------------------------------------------
bool vtkObjectManager::RegisterState(const std::string& state)
{
  using json = nlohmann::json;
  auto stateJson = json::parse(state, nullptr, false);
  if (stateJson.is_discarded())
  {
    vtkErrorMacro(<< "Failed to parse state!");
    return false;
  }
  return this->RegisterState(stateJson);
}

//------------------------------------------------------------------------------
bool vtkObjectManager::RegisterState(const nlohmann::json& stateJson)
{
  if (!this->Context->RegisterState(stateJson))
  {
    vtkErrorMacro(<< "Failed to register state!");
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkObjectManager::UnRegisterState(vtkTypeUInt32 identifier)
{
  return this->Context->UnRegisterState(identifier);
}

//------------------------------------------------------------------------------
void vtkObjectManager::Clear()
{
  this->Context = vtk::TakeSmartPointer(vtkMarshalContext::New());
  this->Deserializer->SetContext(this->Context);
  this->Serializer->SetContext(this->Context);
}

//------------------------------------------------------------------------------
std::string vtkObjectManager::Invoke(
  vtkTypeUInt32 identifier, const std::string& methodName, const std::string& args)
{
  using json = nlohmann::json;
  auto argsJson = json::parse(args, nullptr, false);
  if (argsJson.is_discarded())
  {
    vtkErrorMacro(<< "Failed to parse state!");
    return {};
  }
  return this->Invoke(identifier, methodName, argsJson).dump();
}

//------------------------------------------------------------------------------
nlohmann::json vtkObjectManager::Invoke(
  vtkTypeUInt32 identifier, const std::string& methodName, const nlohmann::json& args)
{
  return this->Invoker->Invoke(identifier, methodName, args);
}

//------------------------------------------------------------------------------
vtkTypeUInt32 vtkObjectManager::GetId(vtkSmartPointer<vtkObjectBase> object)
{
  return this->Context->GetId(object);
}

//------------------------------------------------------------------------------
std::string vtkObjectManager::GetState(vtkTypeUInt32 identifier)
{
  auto state = this->Context->GetState(identifier);
  if (!state.empty() && !state.is_null())
  {
    return state.dump();
  }
  else
  {
    return "";
  }
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkObjectBase> vtkObjectManager::GetObjectAtId(vtkTypeUInt32 identifier)
{
  return this->Context->GetObjectAtId(identifier);
}

//------------------------------------------------------------------------------
std::vector<std::string> vtkObjectManager::GetBlobHashes(const std::vector<vtkTypeUInt32>& ids)
{
  std::vector<std::string> hashes;
  const auto& states = this->Context->States();
  if (states.empty())
  {
    return {};
  }
  for (const auto& id : ids)
  {
    auto stateIter = states.find(std::to_string(id));
    if (stateIter != states.end())
    {
      const auto iter = stateIter.value().find("Hash");
      if (iter != stateIter.value().end())
      {
        hashes.emplace_back(iter->get<std::string>());
      }
      else
      {
        // not uncommon for some objects to have any blobs.
        vtkVLog(this->GetObjectManagerLogVerbosity(), << "Failed to get hash at id=" << id << ".");
      }
    }
    else
    {
      vtkWarningMacro(<< "There is no state at id=" << id << ".");
    }
  }
  return hashes;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkTypeUInt8Array> vtkObjectManager::GetBlob(const std::string& hash) const
{
  return this->Context->GetBlob(hash);
}

//------------------------------------------------------------------------------
bool vtkObjectManager::RegisterBlob(
  const std::string& hash, vtkSmartPointer<vtkTypeUInt8Array> blob)
{
  std::string hashText = hash;
  return this->Context->RegisterBlob(blob, hashText);
}

//------------------------------------------------------------------------------
bool vtkObjectManager::UnRegisterBlob(const std::string& hash)
{
  return this->Context->UnRegisterBlob(hash);
}

//------------------------------------------------------------------------------
void vtkObjectManager::PruneUnusedStates()
{
  // Clear out states that correspond to stale objects
  std::vector<vtkTypeUInt32> staleIds;
  staleIds.reserve(this->Context->WeakObjects().size());
  for (const auto& iter : this->Context->WeakObjects())
  {
    if (iter.second == nullptr)
    {
      staleIds.emplace_back(iter.first);
      vtkVLog(this->GetObjectManagerLogVerbosity(), << "Remove stale state: " << iter.first);
    }
  }
  for (const auto& identifier : staleIds)
  {
    this->Context->UnRegisterState(identifier);
  }
}

//------------------------------------------------------------------------------
void vtkObjectManager::PruneUnusedObjects()
{
  // Find strong objects not referenced by states
  vtkMarshalContext::StrongObjectStore staleStrongObjects;
  for (const auto& iter : this->Context->StrongObjects())
  {
    for (const auto& object : iter.second)
    {
      auto identifier = this->Context->GetId(object);
      auto key = std::to_string(identifier);
      if (!this->Context->States().contains(key))
      {
        staleStrongObjects[iter.first].insert(object);
      }
    }
  }
  for (auto& iter : staleStrongObjects)
  {
    for (const auto& object : iter.second)
    {
      vtkVLog(this->GetObjectManagerLogVerbosity(),
        << "Remove stale strong object: " << iter.first << ":" << object);
      this->Context->Retire(iter.first, object);
    }
  }
  staleStrongObjects.clear();
  // Clear out stale weak references to objects
  std::vector<vtkTypeUInt32> staleIds;
  staleIds.reserve(this->Context->WeakObjects().size());
  for (const auto& iter : this->Context->WeakObjects())
  {
    if (iter.second == nullptr)
    {
      staleIds.emplace_back(iter.first);
      vtkVLog(this->GetObjectManagerLogVerbosity(), << "Remove stale object: " << iter.first);
    }
  }
  for (const auto& identifier : staleIds)
  {
    this->Context->UnRegisterObject(identifier);
  }
}

//------------------------------------------------------------------------------
void vtkObjectManager::PruneUnusedBlobs()
{
  std::unordered_set<std::string> unUsedHashes;
  for (const auto& iter : this->Context->Blobs().items())
  {
    unUsedHashes.insert(iter.key());
  }
  for (const auto& iter : this->Context->States().items())
  {
    const auto& state = iter.value();
    auto hashIter = state.find("Hash");
    if (hashIter != state.end())
    {
      unUsedHashes.erase(hashIter->get<std::string>());
    }
  }
  for (const auto& hash : unUsedHashes)
  {
    this->Context->UnRegisterBlob(hash);
  }
}

//------------------------------------------------------------------------------
std::vector<vtkTypeUInt32> vtkObjectManager::GetAllDependencies(vtkTypeUInt32 identifier)
{
  auto root = identifier;
  std::deque<vtkTypeUInt32> traversealDeque;
  std::unordered_set<vtkTypeUInt32> visited;
  std::vector<vtkTypeUInt32> result;
  traversealDeque.push_back(root);
  while (!traversealDeque.empty())
  {
    const auto front = traversealDeque.front();
    traversealDeque.pop_front();
    if (visited.insert(front).second &&
      (front != vtkObjectManager::ROOT())) // avoids placing the 0 in result
    {
      result.emplace_back(front);
    }
    for (auto& dep : this->Context->GetDirectDependencies(front))
    {
      if (visited.count(dep) == 0)
      {
        traversealDeque.push_back(dep);
      }
    }
  }
  return result;
}

//------------------------------------------------------------------------------
void vtkObjectManager::UpdateObjectsFromStates()
{
  // Reset dependency cache as it will be rebuilt.
  this->Context->ResetDirectDependencies();
  // All objects go under the top level root node
  vtkMarshalContext::ScopedParentTracker rootNodeTracker(this->Context, vtkObjectManager::ROOT());
  // only deserialize those objects which are strong references
  nlohmann::json strongRefStates;
  const auto& states = this->Context->States();
  std::copy_if(states.begin(), states.end(), std::back_inserter(strongRefStates),
    [](const nlohmann::json& item)
    {
      return item.contains("vtk-object-manager-kept-alive") &&
        item["vtk-object-manager-kept-alive"] == true;
    });
  const auto deserializerOwnershipKey = this->Deserializer->GetObjectDescription();
  for (const auto& state : strongRefStates)
  {
    const auto identifier = state.at("Id").get<vtkTypeUInt32>();
    auto object = this->Context->GetObjectAtId(identifier);
    this->Deserializer->DeserializeJSON(identifier, object);
    this->Context->KeepAlive(deserializerOwnershipKey, object);
  }
  // Remove unused objects
  this->PruneUnusedObjects();
  // Remove unused states
  this->PruneUnusedStates();
}

//------------------------------------------------------------------------------
void vtkObjectManager::UpdateStatesFromObjects()
{
  // Reset dependency cache as it will be rebuilt.
  this->Context->ResetDirectDependencies();
  // All objects go under the top level root node
  vtkMarshalContext::ScopedParentTracker rootNodeTracker(this->Context, vtkObjectManager::ROOT());
  // serializes all objects with strong references held by the manager.
  const auto managerStrongObjectsIter = this->Context->StrongObjects().find(this->OWNERSHIP_KEY());
  // serializes all objects with strong references held by the deserializer.
  const auto deserializerOwnershipKey = this->Deserializer->GetObjectDescription();
  const auto deserStrongObjectsIter = this->Context->StrongObjects().find(deserializerOwnershipKey);
  // serializes all objects with strong references held by the invoker.
  const auto invokerOwnershipKey = this->Invoker->GetObjectDescription();
  const auto invokerStrongObjectsIter = this->Context->StrongObjects().find(invokerOwnershipKey);
  if (managerStrongObjectsIter != this->Context->StrongObjects().end())
  {
    for (const auto& object : managerStrongObjectsIter->second)
    {
      auto stateId = this->Serializer->SerializeJSON(object);
      auto idIter = stateId.find("Id");
      if ((idIter != stateId.end()) && idIter->is_number_unsigned())
      {
        auto& state = this->Context->GetState(idIter->get<vtkTypeUInt32>());
        state["vtk-object-manager-kept-alive"] = true;
      }
    }
  }
  if (deserStrongObjectsIter != this->Context->StrongObjects().end())
  {
    for (const auto& object : deserStrongObjectsIter->second)
    {
      this->Serializer->SerializeJSON(object);
    }
  }
  if (invokerStrongObjectsIter != this->Context->StrongObjects().end())
  {
    for (const auto& object : invokerStrongObjectsIter->second)
    {
      this->Serializer->SerializeJSON(object);
    }
  }
  // Remove unused states
  this->PruneUnusedStates();
  // Remove unused objects
  this->PruneUnusedObjects();
}

//------------------------------------------------------------------------------
void vtkObjectManager::UpdateStatesFromObjects(const std::vector<vtkTypeUInt32>& identifiers)
{
  // get objects with strong references held by the manager.
  const auto managerOwnershipKey = this->OWNERSHIP_KEY();
  const auto managerStrongObjectsIter = this->Context->StrongObjects().find(managerOwnershipKey);
  // get objects with strong references held by the deserializer.
  const auto deserializerOwnershipKey = this->Deserializer->GetObjectDescription();
  const auto deserStrongObjectsIter = this->Context->StrongObjects().find(deserializerOwnershipKey);
  // get objects with strong references held by the invoker.
  const auto invokerOwnershipKey = this->Invoker->GetObjectDescription();
  const auto invokerStrongObjectsIter = this->Context->StrongObjects().find(invokerOwnershipKey);

  // for each identifier, serialize the object and mark it as kept alive where necessary.
  for (const auto& identifier : identifiers)
  {
    const auto dependencies = this->GetAllDependencies(identifier);
    for (const auto& depId : dependencies)
    {
      // Reset dependency cache as it will be rebuilt.
      this->Context->ResetDirectDependenciesForNode(depId);
    }
    // The concered strong objects go under the top level root node
    vtkMarshalContext::ScopedParentTracker rootNodeTracker(this->Context, vtkObjectManager::ROOT());
    if (managerStrongObjectsIter != this->Context->StrongObjects().end())
    {
      for (const auto& object : managerStrongObjectsIter->second)
      {
        // The object must have already been registered in the context and have a valid identifier.
        if (this->Context->GetId(object) == identifier)
        {
          this->Serializer->SerializeJSON(object);
        }
      }
    }
    if (deserStrongObjectsIter != this->Context->StrongObjects().end())
    {
      for (const auto& object : deserStrongObjectsIter->second)
      {
        if (this->Context->GetId(object) == identifier)
        {
          this->Serializer->SerializeJSON(object);
        }
      }
    }
    if (invokerStrongObjectsIter != this->Context->StrongObjects().end())
    {
      for (const auto& object : invokerStrongObjectsIter->second)
      {
        if (this->Context->GetId(object) == identifier)
        {
          this->Serializer->SerializeJSON(object);
        }
      }
    }
  }
  // Remove unused states
  this->PruneUnusedStates();
  // Remove unused objects
  this->PruneUnusedObjects();

  // Tag strong objects as kept alive.
  // This is important for the deserializer to know that the object is kept alive.
  // This is done after the serialization of all objects. Otherwise, the serialization of a nested
  // strong object will discard the "vtk-object-manager-kept-alive" tag.
  if (managerStrongObjectsIter != this->Context->StrongObjects().end())
  {
    for (const auto& object : managerStrongObjectsIter->second)
    {
      // The object must have already been registered in the context and have a valid identifier.
      if (auto identifier = this->Context->GetId(object))
      {
        auto& state = this->Context->GetState(identifier);
        state["vtk-object-manager-kept-alive"] = true;
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkObjectManager::UpdateObjectFromState(const std::string& state)
{
  using json = nlohmann::json;
  auto stateJson = json::parse(state, nullptr, false);
  if (stateJson.is_discarded())
  {
    vtkErrorMacro(<< "Failed to parse state=" << state);
    return;
  }
  this->UpdateObjectFromState(stateJson);
}

//------------------------------------------------------------------------------
void vtkObjectManager::UpdateObjectFromState(const nlohmann::json& stateJson)
{
  const auto identifier = stateJson.at("Id").get<vtkTypeUInt32>();
  if (!this->Context->RegisterState(stateJson))
  {
    vtkErrorMacro(<< "Failed to register state=" << stateJson.dump());
    return;
  }
  auto object = this->Context->GetObjectAtId(identifier);
  if (object)
  {
    // clear dependency tree for this object.
    // This lets the deserializer see that the object is not processed
    // in the marshalling context.
    this->Context->ResetDirectDependenciesForNode(identifier);
  }
  if (!this->Deserializer->DeserializeJSON(identifier, object))
  {
    vtkErrorMacro(<< "Failed to update object at id=" << identifier
                  << " from state=" << stateJson.dump());
  }
  else
  {
    vtkVLog(
      this->GetObjectManagerLogVerbosity(), << "Updated object for state at id=" << identifier);
  }
}

//------------------------------------------------------------------------------
void vtkObjectManager::UpdateStateFromObject(vtkTypeUInt32 identifier)
{
  if (auto object = this->Context->GetObjectAtId(identifier))
  {
    // clear dependency tree for this object.
    // This lets the serializer see that the object is not processed
    // in the marshalling context.
    this->Context->ResetDirectDependenciesForNode(identifier);
    const auto id = this->Serializer->SerializeJSON(object);
    if (id.empty())
    {
      vtkErrorMacro(<< "Failed to update state for object at id=" << identifier);
    }
    else
    {
      vtkVLog(
        this->GetObjectManagerLogVerbosity(), << "Updated state for object at id=" << identifier);
    }
  }
  else
  {
    vtkErrorMacro(<< "Cannot update state for object at id=" << identifier
                  << " because there is no such object!");
  }
}

//------------------------------------------------------------------------------
void vtkObjectManager::SetObjectManagerLogVerbosity(vtkLogger::Verbosity verbosity)
{
  this->ObjectManagerLogVerbosity = verbosity;
}

//------------------------------------------------------------------------------
vtkLogger::Verbosity vtkObjectManager::GetObjectManagerLogVerbosity()
{
  // initialize the log verbosity if it is invalid.
  if (this->ObjectManagerLogVerbosity == vtkLogger::VERBOSITY_INVALID)
  {
    this->ObjectManagerLogVerbosity = vtkLogger::VERBOSITY_TRACE;
    // Find an environment variable that specifies logger verbosity
    const char* verbosityKey = "VTK_OBJECT_MANAGER_LOG_VERBOSITY";
    if (vtksys::SystemTools::HasEnv(verbosityKey))
    {
      const char* verbosityCStr = vtksys::SystemTools::GetEnv(verbosityKey);
      const auto verbosity = vtkLogger::ConvertToVerbosity(verbosityCStr);
      if (verbosity > vtkLogger::VERBOSITY_INVALID)
      {
        this->ObjectManagerLogVerbosity = verbosity;
      }
    }
  }
  return this->ObjectManagerLogVerbosity;
}
VTK_ABI_NAMESPACE_END
