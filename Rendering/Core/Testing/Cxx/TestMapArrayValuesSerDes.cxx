// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDeserializer.h"
#include "vtkInvoker.h"
#include "vtkMapArrayValues.h"
#include "vtkMarshalContext.h"
#include "vtkNew.h"
#include "vtkSerializer.h"
#include "vtkSmartPointer.h"
#include "vtkVariant.h"

#include <iostream>
#include <map>

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
vtkSmartPointer<vtkMapArrayValues> RoundTrip(vtkMapArrayValues* source)
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
  // instead of handing back the one already registered against this id.
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
  return vtkMapArrayValues::SafeDownCast(restored);
}

/**
 * A default-constructed object leaves InputArrayName null. Serializing it must not crash and
 * the key must simply be absent.
 */
bool TestDefaultConstructed()
{
  bool success = true;
  vtkNew<vtkMapArrayValues> source;
  EXPECT(source->GetInputArrayName() == nullptr, "expected a null InputArrayName by default");

  auto restored = RoundTrip(source);
  if (!restored)
  {
    std::cerr << "FAIL: round trip of a default-constructed object failed" << std::endl;
    return false;
  }
  EXPECT(restored->GetInputArrayName() == nullptr, "InputArrayName should still be null");
  EXPECT(restored->GetMapSize() == 0, "expected an empty map");
  return success;
}

/**
 * Every scalar property must survive, use values different than what the constructor inits to.
 */
bool TestProperties()
{
  bool success = true;
  vtkNew<vtkMapArrayValues> source;
  source->SetFieldType(vtkMapArrayValues::CELL_DATA); // default POINT_DATA
  source->SetPassArray(1);                            // default 0
  source->SetFillValue(42.5);                         // default -1
  source->SetInputArrayName("input");                 // default nullptr
  source->SetOutputArrayName("output");               // default "ArrayMap"
  source->SetOutputArrayType(VTK_DOUBLE);             // default VTK_INT

  auto restored = RoundTrip(source);
  if (!restored)
  {
    std::cerr << "FAIL: round trip of a configured object failed" << std::endl;
    return false;
  }

  EXPECT(restored->GetFieldType() == vtkMapArrayValues::CELL_DATA, "FieldType is incorrect");
  EXPECT(restored->GetPassArray() == 1, "PassArray is incorrect");
  EXPECT(restored->GetFillValue() == 42.5, "FillValue is incorrect");
  EXPECT(restored->GetInputArrayName() != nullptr &&
      std::string(restored->GetInputArrayName()) == "input",
    "InputArrayName is incorrect");
  EXPECT(restored->GetOutputArrayName() != nullptr &&
      std::string(restored->GetOutputArrayName()) == "output",
    "OutputArrayName is incorrect");
  EXPECT(restored->GetOutputArrayType() == VTK_DOUBLE, "OutputArrayType is incorrect");
  return success;
}

/**
 * The map itself, with a mix of variant types so more than one branch of
 * Serialize_vtkVariant is exercised. Several entries also catch a map that collapses to a
 * single element.
 */
bool TestMapContents()
{
  bool success = true;
  vtkNew<vtkMapArrayValues> source;
  source->AddToMap(vtkVariant("alpha"), vtkVariant("beta")); // string -> string
  source->AddToMap(vtkVariant(7), vtkVariant(9));            // integer -> integer
  source->AddToMap(vtkVariant(1.25), vtkVariant("gamma"));   // double -> string
  source->AddToMap(vtkVariant("delta"), vtkVariant(3.5));    // string -> double
  const int expectedSize = source->GetMapSize();
  EXPECT(expectedSize == 4, "expected 4 entries in the source map");

  auto restored = RoundTrip(source);
  if (!restored)
  {
    std::cerr << "FAIL: round trip of a populated map failed" << std::endl;
    return false;
  }

  EXPECT(restored->GetMapSize() == expectedSize, "map size is incorrect");

  const auto& original = source->GetMap();
  const auto& copy = restored->GetMap();
  for (const auto& entry : original)
  {
    const auto it = copy.find(entry.first);
    if (it == copy.end())
    {
      std::cerr << "FAIL: key " << entry.first.ToString() << " missing after round trip"
                << std::endl;
      success = false;
      continue;
    }
    // Compare type as well as value: a variant that decayed to another type (a string "7"
    // instead of the integer 7, say) still compares equal under vtkVariant::operator==.
    EXPECT(it->second.GetType() == entry.second.GetType(),
      "value type changed for key " << entry.first.ToString());
    EXPECT(it->second.ToString() == entry.second.ToString(),
      "value changed for key " << entry.first.ToString());
    EXPECT(it->first.GetType() == entry.first.GetType(),
      "key type changed for key " << entry.first.ToString());
  }
  return success;
}

} // anon namespace

int TestMapArrayValuesSerDes(int, char*[])
{
  bool success = true;
  success &= TestDefaultConstructed();
  success &= TestProperties();
  success &= TestMapContents();
  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
