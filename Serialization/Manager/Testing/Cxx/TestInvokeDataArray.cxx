// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAffineTypeFloat32Array.h"
#include "vtkBitArray.h"
#include "vtkConstantTypeFloat32Array.h"
#include "vtkFloatArray.h"
#include "vtkInvoker.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkObjectManager.h"

// clang-format off
#include "vtkType.h"
#include "vtk_nlohmannjson.h"
#include VTK_NLOHMANN_JSON(json.hpp)
// clang-format on

#include <cstdint>
#include <cstdlib>

namespace
{
using json = nlohmann::json;

bool CheckEqual(const json& actual, const json& expected, const std::string& what)
{
  if (actual != expected)
  {
    vtkLog(ERROR, << what << ": got " << actual.dump() << ", expected " << expected.dump());
    return false;
  }
  return true;
}

/**
 * Invoke a method that is expected to succeed and compare the returned value.
 */
bool CheckInvoke(vtkObjectManager* manager, vtkTypeUInt32 identifier, const char* methodName,
  const json& args, const json& expected)
{
  const auto value = manager->Invoke(identifier, methodName, args);
  return CheckEqual(value, expected, std::string(methodName) + args.dump());
}

/**
 * Invoke a method that is expected to succeed and ignore the returned value.
 */
bool CheckInvoke(
  vtkObjectManager* manager, vtkTypeUInt32 identifier, const char* methodName, const json& args)
{
  // the invoker is used directly here because vtkObjectManager::Invoke drops the status.
  const auto result = manager->GetInvoker()->Invoke(identifier, methodName, args);
  if (!result.value("Success", false))
  {
    vtkLog(ERROR, << "Expected " << methodName << args.dump() << " to succeed: " << result.dump());
    return false;
  }
  return true;
}

/**
 * Invoke a method that is expected to fail. The invoker is used directly to avoid the
 * error message that vtkObjectManager emits for a failed invocation.
 */
bool CheckInvokeFails(
  vtkObjectManager* manager, vtkTypeUInt32 identifier, const char* methodName, const json& args)
{
  const auto result = manager->GetInvoker()->Invoke(identifier, methodName, args);
  if (result.value("Success", false))
  {
    vtkLog(ERROR, << "Expected " << methodName << args.dump() << " to fail: " << result.dump());
    return false;
  }
  if (result.value("Message", std::string()).empty())
  {
    vtkLog(ERROR, << "Expected " << methodName << args.dump() << " to report a message");
    return false;
  }
  return true;
}

bool TestAOSDataArray(vtkObjectManager* manager)
{
  bool success = true;
  vtkNew<vtkFloatArray> array;
  array->SetNumberOfComponents(3);
  array->SetNumberOfTuples(2);
  array->Fill(0.f);
  const auto id = manager->RegisterObject(array);

  // typed value accessors
  success &= CheckInvoke(manager, id, "SetValue", json::array({ 0, 42.5 }));
  success &= CheckInvoke(manager, id, "GetValue", json::array({ 0 }), 42.5);

  // typed component accessors
  success &= CheckInvoke(manager, id, "SetTypedComponent", json::array({ 1, 2, -1.25 }));
  success &= CheckInvoke(manager, id, "GetTypedComponent", json::array({ 1, 2 }), -1.25);
  success &= CheckInvoke(manager, id, "GetValue", json::array({ 5 }), -1.25);

  // typed tuple accessors
  success &= CheckInvoke(manager, id, "SetTypedTuple", json::array({ 0, { 1.5, 2.5, 3.5 } }));
  success &=
    CheckInvoke(manager, id, "GetTypedTuple", json::array({ 0 }), json::array({ 1.5, 2.5, 3.5 }));

  // the address of the array. The type of the elements behind it is in the type manifest.
  success &= CheckInvoke(manager, id, "GetPointer", json::array({ 0 }),
    reinterpret_cast<std::uintptr_t>(array->GetPointer(0)));

  // hand a foreign buffer over to the array without transferring ownership.
  std::vector<float> foreignStore = { 10.f, 11.f, 12.f, 13.f };
  const auto address = reinterpret_cast<std::uintptr_t>(foreignStore.data());
  success &= CheckInvoke(manager, id, "SetArray", json::array({ address, 4, 1 }));
  success &= CheckInvoke(manager, id, "GetValue", json::array({ 3 }), 13.0);
  if (array->GetPointer(0) != foreignStore.data())
  {
    vtkLog(ERROR, << "SetArray did not take the supplied buffer");
    success = false;
  }

  // an index that the method does not expect is rejected instead of corrupting memory. The array
  // holds 4 values at this point, so the second tuple is out of reach.
  success &= CheckInvokeFails(manager, id, "GetTypedTuple", json::array({ 2 }));
  success &= CheckInvokeFails(manager, id, "SetTypedTuple", json::array({ 2, { 0.f, 0.f, 0.f } }));
  success &= CheckInvokeFails(manager, id, "GetValue", json::array({ 4 }));
  success &= CheckInvokeFails(manager, id, "SetValue", json::array({ 4, 0.f }));
  // arguments that do not match the signature are rejected.
  success &= CheckInvokeFails(manager, id, "GetValue", json::array({ 0, 0 }));
  success &= CheckInvokeFails(manager, id, "GetValue", json::array({ "zero" }));
  success &= CheckInvokeFails(manager, id, "SetValue", json::array({ 0, "one" }));
  success &= CheckInvokeFails(manager, id, "SetTypedTuple", json::array({ 0, { 1.5, 2.5 } }));
  success &= CheckInvokeFails(manager, id, "SetArray", json::array({ 0, 4, 1 }));
  success &= CheckInvokeFails(manager, id, "GetPointer", json::array({}));
  success &= CheckInvokeFails(manager, id, "NoSuchMethodOnAnArray", json::array({}));

  // methods of vtkDataArray are still reachable through the same handler.
  success &= CheckInvoke(manager, id, "GetNumberOfComponents", json::array({}), 3);

  // the array does not own the buffer, so it is safe to free it here.
  manager->UnRegisterObject(id);
  return success;
}

bool TestBitArray(vtkObjectManager* manager)
{
  bool success = true;
  vtkNew<vtkBitArray> array;
  array->SetNumberOfComponents(1);
  array->SetNumberOfTuples(8);
  for (vtkIdType i = 0; i < array->GetNumberOfValues(); ++i)
  {
    array->SetValue(i, 0);
  }
  const auto id = manager->RegisterObject(array);

  // a bit value is an integer, as in the signature of the method.
  success &= CheckInvoke(manager, id, "SetValue", json::array({ 0, 1 }));
  success &= CheckInvoke(manager, id, "GetValue", json::array({ 0 }), 1);
  success &= CheckInvoke(manager, id, "SetValue", json::array({ 1, 0 }));
  success &= CheckInvoke(manager, id, "GetValue", json::array({ 1 }), 0);
  success &= CheckInvoke(manager, id, "SetTypedComponent", json::array({ 2, 0, 1 }));
  success &= CheckInvoke(manager, id, "GetTypedComponent", json::array({ 2, 0 }), 1);
  // a tuple travels as an array of as many elements as the array has components.
  success &= CheckInvoke(manager, id, "SetTypedTuple", json::array({ 3, { 0 } }));
  success &= CheckInvoke(manager, id, "GetTypedTuple", json::array({ 3 }), json::array({ 0 }));

  // LookupValue finds the first set bit.
  success &= CheckInvoke(manager, id, "LookupValue", json::array({ 1 }), 0);

  // insertion grows the array.
  success &= CheckInvoke(manager, id, "InsertNextValue", json::array({ 1 }), 8);
  success &= CheckInvoke(manager, id, "GetValue", json::array({ 8 }), 1);
  success &= CheckInvoke(manager, id, "InsertValue", json::array({ 15, 1 }));
  success &= CheckInvoke(manager, id, "GetValue", json::array({ 15 }), 1);

  success &= CheckInvoke(manager, id, "GetPointer", json::array({ 0 }),
    reinterpret_cast<std::uintptr_t>(array->GetPointer(0)));

  // hand a foreign buffer of 8 bits over to the array without transferring ownership.
  std::vector<unsigned char> foreignStore = { 0xa0, 0, 0, 0, 0, 0, 0, 0 };
  const auto address = reinterpret_cast<std::uintptr_t>(foreignStore.data());
  success &= CheckInvoke(manager, id, "SetArray", json::array({ address, 8, 1 }));
  success &= CheckInvoke(manager, id, "GetValue", json::array({ 0 }), 1);
  success &= CheckInvoke(manager, id, "GetValue", json::array({ 1 }), 0);
  success &= CheckInvoke(manager, id, "GetValue", json::array({ 2 }), 1);

  success &= CheckInvokeFails(manager, id, "SetValue", json::array({ 0, "one" }));
  success &= CheckInvokeFails(manager, id, "SetTypedTuple", json::array({ 0, { 1, 1 } }));
  success &= CheckInvokeFails(manager, id, "GetTypedTuple", json::array({ 8 }));
  success &= CheckInvokeFails(manager, id, "NoSuchMethodOnABitArray", json::array({}));

  // the array does not own the buffer, so it is safe to free it here.
  manager->UnRegisterObject(id);
  return success;
}

bool TestImplicitArrays(vtkObjectManager* manager)
{
  bool success = true;
  vtkNew<vtkAffineTypeFloat32Array> affine;
  affine->ConstructBackend(2.f, 3.f);
  affine->SetNumberOfComponents(1);
  affine->SetNumberOfTuples(4);
  const auto affineId = manager->RegisterObject(affine);

  success &= CheckInvoke(manager, affineId, "GetSlope", json::array({}), 2.0);
  success &= CheckInvoke(manager, affineId, "GetIntercept", json::array({}), 3.0);
  success &= CheckInvokeFails(manager, affineId, "GetSlope", json::array({ 0 }));
  success &= CheckInvokeFails(manager, affineId, "NoSuchMethodOnAnAffineArray", json::array({}));
  // the backend of an implicit array is parameterized through the invoker. The values that the
  // array reports follow from it.
  success &= CheckInvoke(manager, affineId, "ConstructBackend", json::array({ 4.0, 5.0 }));
  success &= CheckInvoke(manager, affineId, "GetSlope", json::array({}), 4.0);
  success &= CheckInvoke(manager, affineId, "GetIntercept", json::array({}), 5.0);
  success &= CheckInvoke(manager, affineId, "GetValue", json::array({ 2 }), 13.0);
  success &= CheckInvokeFails(manager, affineId, "ConstructBackend", json::array({ 4.0 }));

  vtkNew<vtkConstantTypeFloat32Array> constant;
  constant->ConstructBackend(7.5f);
  constant->SetNumberOfComponents(1);
  constant->SetNumberOfTuples(4);
  const auto constantId = manager->RegisterObject(constant);

  success &= CheckInvoke(manager, constantId, "GetConstantValue", json::array({}), 7.5);
  success &= CheckInvokeFails(manager, constantId, "GetConstantValue", json::array({ 0 }));
  success &= CheckInvokeFails(manager, constantId, "NoSuchMethodOnAConstantArray", json::array({}));
  // the backend of an implicit array is parameterized through the invoker. The values that the
  // array reports follow from it.
  success &= CheckInvoke(manager, constantId, "ConstructBackend", json::array({ 8.5 }));
  success &= CheckInvoke(manager, constantId, "GetConstantValue", json::array({}), 8.5);
  success &= CheckInvoke(manager, constantId, "GetValue", json::array({ 3 }), 8.5);
  success &= CheckInvokeFails(manager, constantId, "ConstructBackend", json::array({ 8.5, 9.5 }));

  manager->UnRegisterObject(affineId);
  manager->UnRegisterObject(constantId);
  return success;
}
} // namespace

int TestInvokeDataArray(int argc, char* argv[])
{
  (void)argc;
  (void)argv;

  auto manager = vtk::TakeSmartPointer(vtkObjectManager::New());
  manager->InitializeDefaultHandlers();
#if !defined(NDEBUG)
  manager->SetObjectManagerLogVerbosity(vtkLogger::VERBOSITY_INFO);
  manager->GetInvoker()->SetInvokerLogVerbosity(vtkLogger::VERBOSITY_INFO);
#endif

  bool success = true;
  success &= TestAOSDataArray(manager);
  success &= TestBitArray(manager);
  success &= TestImplicitArrays(manager);
  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
