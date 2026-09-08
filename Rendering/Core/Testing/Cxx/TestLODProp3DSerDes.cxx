// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCommand.h"
#include "vtkConeSource.h"
#include "vtkDeserializer.h"
#include "vtkInvoker.h"
#include "vtkLODProp3D.h"
#include "vtkMarshalContext.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkProp3D.h"
#include "vtkPropCollection.h"
#include "vtkProperty.h"
#include "vtkSerializer.h"
#include "vtkSmartPointer.h"

#include <iostream>

namespace
{

#define EXPECT(cond, msg)                                                                          \
  do                                                                                               \
  {                                                                                                \
    if (!(cond))                                                                                   \
    {                                                                                              \
      std::cerr << "FAIL: " << msg << std::endl;                                                   \
      success = false;                                                                             \
    }                                                                                              \
  } while (0)

/**
 * Serialize `source`, then deserialize the resulting state into a fresh object.
 * Returns nullptr when either half of the round trip fails.
 */
vtkSmartPointer<vtkLODProp3D> RoundTrip(vtkLODProp3D* source)
{
  vtkNew<vtkMarshalContext> context;
  vtkNew<vtkSerializer> serializer;
  vtkNew<vtkDeserializer> deserializer;
  vtkNew<vtkInvoker> invoker;
  serializer->SetContext(context);
  deserializer->SetContext(context);

  if (const char* error = nullptr;
      !context->CallRegistrars(serializer, deserializer, invoker, &error))
  {
    std::cerr << "Failed to call registrars: " << error << std::endl;
    return nullptr;
  }

  const auto result = serializer->SerializeJSON(source);
  auto idIt = result.find("Id");
  if (idIt == result.end() || !idIt->is_number_unsigned())
  {
    std::cerr << "No 'Id' in serialized state." << std::endl;
    return nullptr;
  }

  // Hand the state back under a fresh identifier so the deserializer builds a new object
  // instead of returning the one already registered against this id.
  auto state = context->GetState(idIt->get<vtkTypeUInt32>());
  const vtkTypeUInt32 newId = context->MakeId();
  state["Id"] = newId;
  if (!context->RegisterState(state))
  {
    std::cerr << "Failed to register state for deserialization." << std::endl;
    return nullptr;
  }

  vtkSmartPointer<vtkObjectBase> restored;
  if (!deserializer->DeserializeJSON(newId, restored))
  {
    std::cerr << "DeserializeJSON failed." << std::endl;
    return nullptr;
  }
  return vtkLODProp3D::SafeDownCast(restored);
}

/**
 * Add an actor style LOD backed by a cone, so each LOD carries a distinguishable mapper.
 */
int AddConeLOD(vtkLODProp3D* prop, double time, double opacity)
{
  vtkNew<vtkConeSource> cone;
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(cone->GetOutputPort());
  vtkNew<vtkProperty> property;
  property->SetOpacity(opacity);
  return prop->AddLOD(mapper, property, time);
}

/**
 * An empty vtkLODProp3D must survive a round trip.
 */
bool TestNoLODs()
{
  bool success = true;
  vtkNew<vtkLODProp3D> source;
  source->SetAutomaticLODSelection(0);     // default is 1
  source->SetAutomaticPickLODSelection(0); // default is 1

  auto restored = RoundTrip(source);
  if (!restored)
  {
    std::cerr << "FAIL: round trip of an empty vtkLODProp3D failed" << std::endl;
    return false;
  }
  EXPECT(restored->GetNumberOfLODs() == 0, "expected no LODs");
  EXPECT(restored->GetAutomaticLODSelection() == 0, "AutomaticLODSelection did not round trip");
  EXPECT(
    restored->GetAutomaticPickLODSelection() == 0, "AutomaticPickLODSelection did not round trip");
  return success;
}

/**
 * The entry list is serialized directly, so IDs are preserved rather than reassigned. Every
 * per-LOD field must come back under its original ID.
 */
bool TestLODs()
{
  bool success = true;
  vtkNew<vtkLODProp3D> source;
  const int idA = AddConeLOD(source, 0.5, 0.25);
  const int idB = AddConeLOD(source, 1.5, 0.50);
  const int idC = AddConeLOD(source, 2.5, 0.75);
  source->SetLODLevel(idA, 3.0);
  source->SetLODLevel(idB, 2.0);
  source->SetLODLevel(idC, 1.0);
  source->DisableLOD(idB);
  EXPECT(source->GetNumberOfLODs() == 3, "expected 3 LODs in the source");

  auto restored = RoundTrip(source);
  if (!restored)
  {
    std::cerr << "FAIL: round trip of a populated vtkLODProp3D failed" << std::endl;
    return false;
  }

  EXPECT(restored->GetNumberOfLODs() == 3, "NumberOfLODs did not round trip");

  for (const int id : { idA, idB, idC })
  {
    EXPECT(restored->GetLODLevel(id) == source->GetLODLevel(id),
      "Level did not round trip for LOD " << id << ": got " << restored->GetLODLevel(id)
                                          << ", expected " << source->GetLODLevel(id));
    EXPECT(restored->GetLODEstimatedRenderTime(id) == source->GetLODEstimatedRenderTime(id),
      "EstimatedRenderTime did not round trip for LOD "
        << id << ": got " << restored->GetLODEstimatedRenderTime(id) << ", expected "
        << source->GetLODEstimatedRenderTime(id));
    EXPECT(restored->IsLODEnabled(id) == source->IsLODEnabled(id),
      "State did not round trip for LOD " << id);

    auto* restoredMapper = restored->GetLODMapper(id);
    EXPECT(restoredMapper != nullptr, "mapper missing for LOD " << id);
    EXPECT(vtkPolyDataMapper::SafeDownCast(restoredMapper) != nullptr,
      "Prop3DType did not round trip for LOD " << id);

    vtkProperty* originalProperty = nullptr;
    vtkProperty* restoredProperty = nullptr;
    source->GetLODProperty(id, &originalProperty);
    restored->GetLODProperty(id, &restoredProperty);
    if (originalProperty && restoredProperty)
    {
      EXPECT(restoredProperty->GetOpacity() == originalProperty->GetOpacity(),
        "property opacity did not round trip for LOD " << id);
    }
    else
    {
      EXPECT(false, "property missing for LOD " << id);
    }
  }
  return success;
}

/**
 * RemoveLOD leaves a hole in the entry array. Serializing the array directly means the hole is
 * carried across as a null Prop3D rather than being compacted away, so the surviving IDs keep
 * their slots.
 */
bool TestHoles()
{
  bool success = true;
  vtkNew<vtkLODProp3D> source;
  const int idA = AddConeLOD(source, 0.5, 0.25);
  const int idB = AddConeLOD(source, 1.5, 0.50);
  const int idC = AddConeLOD(source, 2.5, 0.75);
  source->SetLODLevel(idA, 3.0);
  source->SetLODLevel(idC, 1.0);

  // Punch a hole in the middle of the entry array.
  source->RemoveLOD(idB);
  EXPECT(source->GetNumberOfLODs() == 2, "expected 2 LODs after RemoveLOD");

  auto restored = RoundTrip(source);
  if (!restored)
  {
    std::cerr << "FAIL: round trip with a removed LOD failed" << std::endl;
    return false;
  }

  EXPECT(restored->GetNumberOfLODs() == 2, "NumberOfLODs did not round trip after RemoveLOD");
  EXPECT(restored->GetLODLevel(idA) == 3.0, "Level lost for the LOD before the hole");
  EXPECT(restored->GetLODLevel(idC) == 1.0, "Level lost for the LOD after the hole");
  EXPECT(restored->GetLODMapper(idA) != nullptr, "mapper lost for the LOD before the hole");
  EXPECT(restored->GetLODMapper(idC) != nullptr, "mapper lost for the LOD after the hole");
  return success;
}

/**
 * Because IDs survive, the two ID valued properties can be restored verbatim, and must still
 * resolve to a live LOD afterwards.
 */
bool TestSelectedIDs()
{
  bool success = true;
  vtkNew<vtkLODProp3D> source;
  const int idA = AddConeLOD(source, 0.5, 0.25);
  AddConeLOD(source, 1.5, 0.50);
  const int idC = AddConeLOD(source, 2.5, 0.75);

  source->SetAutomaticLODSelection(0);
  source->SetSelectedLODID(idC);
  source->SetAutomaticPickLODSelection(0);
  source->SetSelectedPickLODID(idA);

  auto restored = RoundTrip(source);
  if (!restored)
  {
    std::cerr << "FAIL: round trip with explicit selections failed" << std::endl;
    return false;
  }

  EXPECT(restored->GetSelectedLODID() == idC,
    "SelectedLODID did not round trip: got " << restored->GetSelectedLODID() << ", expected "
                                             << idC);
  EXPECT(restored->GetSelectedPickLODID() == idA,
    "SelectedPickLODID did not round trip: got " << restored->GetSelectedPickLODID()
                                                 << ", expected " << idA);
  EXPECT(restored->GetLODMapper(restored->GetSelectedLODID()) != nullptr,
    "SelectedLODID does not resolve to a LOD");
  return success;
}

/**
 * A LOD added after deserializing must not reuse an ID that already belongs to a restored LOD,
 * which means vtkLODProp3D::CurrentIndex has to be restored along with the entries.
 */
bool TestIDsStayUniqueAfterRestore()
{
  bool success = true;
  vtkNew<vtkLODProp3D> source;
  const int idA = AddConeLOD(source, 0.5, 0.25);
  const int idB = AddConeLOD(source, 1.5, 0.50);

  auto restored = RoundTrip(source);
  if (!restored)
  {
    std::cerr << "FAIL: round trip failed" << std::endl;
    return false;
  }

  const int idNew = AddConeLOD(restored, 3.5, 1.0);
  EXPECT(
    idNew != idA && idNew != idB, "a LOD added after restoring reused an existing ID: " << idNew);
  EXPECT(restored->GetNumberOfLODs() == 3, "adding a LOD after restoring did not grow the list");
  // The pre-existing LODs must still be reachable after the list grew.
  EXPECT(restored->GetLODMapper(idA) != nullptr, "restored LOD lost after adding another");
  EXPECT(restored->GetLODMapper(idB) != nullptr, "restored LOD lost after adding another");
  return success;
}

/**
 * AddLOD registers the owning prop as a consumer of each LOD's prop and attaches the pick
 * observer. Restoring the entry array directly bypasses AddLOD, so that wiring has to be
 * re-established explicitly or picking silently stops working on restored LODs.
 */
bool TestPropsAreReattached()
{
  bool success = true;
  vtkNew<vtkLODProp3D> source;
  AddConeLOD(source, 0.5, 0.25);
  AddConeLOD(source, 1.5, 0.50);

  auto restored = RoundTrip(source);
  if (!restored)
  {
    std::cerr << "FAIL: round trip failed" << std::endl;
    return false;
  }

  vtkNew<vtkPropCollection> actors;
  restored->GetActors(actors);
  EXPECT(actors->GetNumberOfItems() == 2, "expected 2 actors, got " << actors->GetNumberOfItems());

  actors->InitTraversal();
  int checked = 0;
  while (vtkProp* prop = actors->GetNextProp())
  {
    EXPECT(prop->IsConsumer(restored) != 0,
      "restored LOD prop " << checked << " does not list the vtkLODProp3D as a consumer");
    EXPECT(prop->HasObserver(vtkCommand::PickEvent) != 0,
      "restored LOD prop " << checked << " has no PickEvent observer");
    ++checked;
  }
  EXPECT(checked == 2, "expected to check 2 props, checked " << checked);
  return success;
}

/**
 * vtkLODPRop3D::CurrentIndex is what stops a retired ID from being handed out again.
 * RebuildLODBookkeeping can infer it from the surviving entries, so the only case that really
 * exercises the serialized value is a prop whose LODs have all been removed: there is no live ID
 * left to infer from.
 */
bool TestCurrentIndexPreserved()
{
  bool success = true;
  vtkNew<vtkLODProp3D> source;
  const int idA = AddConeLOD(source, 0.5, 0.25);
  const int idB = AddConeLOD(source, 1.5, 0.50);
  const int idC = AddConeLOD(source, 2.5, 0.75);

  // Point the selection at a LOD, then retire every one of them.
  source->SetAutomaticLODSelection(0);
  source->SetSelectedLODID(idA);
  source->RemoveLOD(idA);
  source->RemoveLOD(idB);
  source->RemoveLOD(idC);
  EXPECT(source->GetNumberOfLODs() == 0, "expected every LOD to be removed");

  auto restored = RoundTrip(source);
  if (!restored)
  {
    std::cerr << "FAIL: round trip with every LOD removed failed" << std::endl;
    return false;
  }

  EXPECT(restored->GetCurrentIndex() == source->GetCurrentIndex(),
    "CurrentIndex did not round trip: got " << restored->GetCurrentIndex() << ", expected "
                                            << source->GetCurrentIndex());

  // Without CurrentIndex the next LOD would reuse a retired ID, and the restored SelectedLODID
  // would silently start pointing at it.
  const int idNew = AddConeLOD(restored, 3.5, 1.0);
  EXPECT(idNew != idA && idNew != idB && idNew != idC,
    "a LOD added after restoring reused a retired ID: " << idNew);
  EXPECT(idNew != restored->GetSelectedLODID(),
    "a LOD added after restoring collided with the restored SelectedLODID: " << idNew);
  return success;
}

/**
 * Deserializing twice into the same object must not grow the entry list or leak the props that
 * the first pass installed.
 */
bool TestIdempotentDeserialize()
{
  bool success = true;
  vtkNew<vtkMarshalContext> context;
  vtkNew<vtkSerializer> serializer;
  vtkNew<vtkDeserializer> deserializer;
  vtkNew<vtkInvoker> invoker;
  serializer->SetContext(context);
  deserializer->SetContext(context);
  if (const char* error = nullptr;
      !context->CallRegistrars(serializer, deserializer, invoker, &error))
  {
    std::cerr << "Failed to call registrars: " << error << std::endl;
    return false;
  }

  vtkNew<vtkLODProp3D> source;
  AddConeLOD(source, 0.5, 0.25);
  AddConeLOD(source, 1.5, 0.50);

  const auto result = serializer->SerializeJSON(source);
  const auto state = context->GetState(result.at("Id").get<vtkTypeUInt32>());

  auto handler = deserializer->GetHandler(typeid(vtkLODProp3D));
  if (!handler)
  {
    std::cerr << "FAIL: no deserialization handler registered for vtkLODProp3D" << std::endl;
    return false;
  }

  vtkNew<vtkLODProp3D> target;
  handler(state, target, deserializer);
  EXPECT(target->GetNumberOfLODs() == 2,
    "expected 2 LODs after the first deserialize, got " << target->GetNumberOfLODs());
  handler(state, target, deserializer);
  EXPECT(target->GetNumberOfLODs() == 2,
    "deserializing twice must not duplicate LODs, got " << target->GetNumberOfLODs());
  return success;
}

} // anonymous namespace

int TestLODProp3DSerDes(int, char*[])
{
  bool success = true;
  success &= TestNoLODs();
  success &= TestLODs();
  success &= TestHoles();
  success &= TestSelectedIDs();
  success &= TestIDsStayUniqueAfterRestore();
  success &= TestPropsAreReattached();
  success &= TestCurrentIndexPreserved();
  success &= TestIdempotentDeserialize();
  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
