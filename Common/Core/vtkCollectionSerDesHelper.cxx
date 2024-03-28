// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCollection.h"

#include "vtkDeserializer.h"
#include "vtkSerializer.h"

// clang-format off
#include "vtk_nlohmannjson.h"
#include VTK_NLOHMANN_JSON(json.hpp)
// clang-format on

extern "C"
{
  int RegisterHandlers_vtkCollectionSerDesHelper(void* ser, void* deser);
}

static nlohmann::json Serialize_vtkCollection(vtkObjectBase* object, vtkSerializer* serializer)
{
  auto collection = vtkCollection::SafeDownCast(object);
  if (!collection)
  {
    return {};
  }
  nlohmann::json state;
  if (auto superSerializer = serializer->GetHandler(typeid(vtkCollection::Superclass)))
  {
    state = superSerializer(object, serializer);
  }
  state["NumberOfItems"] = collection->GetNumberOfItems();
  auto& dst = state["Items"] = nlohmann::json::array();
  vtkCollectionSimpleIterator cookie;
  collection->InitTraversal(cookie);
  while (auto* item = collection->GetNextItemAsObject(cookie))
  {
    // vtkWidgetRepresentation is typically already serialized as part of a vtkAbstractWidget.
    if (item->IsA("vtkWidgetRepresentation"))
    {
      continue;
    }
    dst.emplace_back(serializer->SerializeJSON(item));
  }
  auto& superClasses = state["SuperClassNames"];
  superClasses.push_back("vtkObject");
  return state;
}

static void Deserialize_vtkCollection(
  const nlohmann::json& state, vtkObjectBase* object, vtkDeserializer* deserializer)
{
  auto collection = vtkCollection::SafeDownCast(object);
  if (!collection)
  {
    return;
  }
  if (const auto superDeserializer = deserializer->GetHandler(typeid(vtkCollection::Superclass)))
  {
    superDeserializer(state, object, deserializer);
  }
  const auto items = state["Items"].get<nlohmann::json::array_t>();
  // if number of items changed, remove all items.
  bool removedItems = false;
  {
    const auto numberOfItems = state["NumberOfItems"].get<int>();
    if (numberOfItems != collection->GetNumberOfItems())
    {
      collection->RemoveAllItems();
      removedItems = true;
    }
  }
  for (const auto& item : items)
  {
    const auto* context = deserializer->GetContext();
    const auto identifier = item["Id"].get<vtkTypeUInt32>();
    auto subObject = context->GetObjectAtId(identifier);
    deserializer->DeserializeJSON(identifier, subObject);
    if (auto* itemAsObject = vtkObject::SafeDownCast(subObject))
    {
      // vtkWidgetRepresentation is typically already de-serialized as part of a vtkAbstractWidget
      if (itemAsObject->IsA("vtkWidgetRepresentation"))
      {
        continue;
      }
      if (removedItems)
      {
        collection->AddItem(itemAsObject);
      }
    }
  }
}

int RegisterHandlers_vtkCollectionSerDesHelper(void* ser, void* deser)
{
  int success = 0;
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(ser))
  {
    if (auto* serializer = vtkSerializer::SafeDownCast(asObjectBase))
    {
      serializer->RegisterHandler(typeid(vtkCollection), Serialize_vtkCollection);
      success = 1;
    }
  }
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(deser))
  {
    if (auto* deserializer = vtkDeserializer::SafeDownCast(asObjectBase))
    {
      deserializer->RegisterHandler(typeid(vtkCollection), Deserialize_vtkCollection);
      deserializer->RegisterConstructor("vtkCollection", []() { return vtkCollection::New(); });
      success = 1;
    }
  }
  return success;
}
