// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkObjectManager.h"
#include "vtkSerDesMock.h"
#include "vtkSerDesMockObject.h"

#include "vtkTestingSerializationSerDes.h"

// clang-format off
#include "vtkType.h"
#include "vtk_nlohmannjson.h"            // for json
#include VTK_NLOHMANN_JSON(json_fwd.hpp) // for json
// clang-format on

#include <cstdlib>

int TestInvoke(int argc, char* argv[])
{
  (void)argc;
  (void)argv;

  auto manager = vtk::TakeSmartPointer(vtkObjectManager::New());
  // Manually register handlers for the vtkTestingSerializationSerDes library
  // because these handlers are not part of the 'default' group.
  manager->InitializeDefaultHandlers();
  manager->InitializeExtensionModuleHandlers({ RegisterClasses_vtkTestingSerialization });
#if !defined(NDEBUG)
  manager->SetObjectManagerLogVerbosity(vtkLogger::VERBOSITY_INFO);
  manager->GetInvoker()->SetInvokerLogVerbosity(vtkLogger::VERBOSITY_INFO);
#endif

  vtkNew<vtkSerDesMockObject> argMockObject;
  argMockObject->SetTag(5678);
  const auto idArgMockObject = manager->RegisterObject(argMockObject);

  constexpr auto newCStyleEnumValue = vtkSerDesMock::Value2;
  constexpr auto newMemberScopedEnumValue = vtkSerDesMock::MemberScopedEnum::Value2;
  const auto newVTKObjectValue = nlohmann::json{ { "Id", idArgMockObject } };
  constexpr double newNumericScalarValue = 2.0;
  const nlohmann::json /*float[4]*/ newNumericArrayValue = { 1.f, 2.f, 3.f, 4.f };
  const nlohmann::json /*char[]*/ newCharPointerValue = "TestInvokeCharPointer";
  const std::string newStdStringValue = "TestInvokeStdString";
  const nlohmann::json /*std::vector<int>*/ newStdVectorOfIntValue{ 1, 2, 3, 4, 5 };
  const nlohmann::json /*std::vector<float>*/ newStdVectorOfRealValue{ 1.f, 2.f, 3.f, 4.f };
  const nlohmann::json /*std::vector<std::string>*/ newStdVectorOfStdStringValue{ "Test", "Invoke",
    "StdVector", "Of", "String" };
  const nlohmann::json /*vtkBoundingBox*/ newBoundingBoxValue{ -10.0, 10.0, -100.0, 100.0, -1000.0,
    1000.0 };
  const nlohmann::json /*vtkColor3d*/ newColor3dValue{ 0.1, 0.2, 0.3 };
  const nlohmann::json /*vtkColor3f*/ newColor3fValue{ 0.1f, 0.2f, 0.3f };
  const nlohmann::json /*vtkColor3ub*/ newColor3ubValue{ 111, 222, 123 };
  const nlohmann::json /*vtkColor4d*/ newColor4dValue{ 0.1, 0.2, 0.3, 0.4 };
  const nlohmann::json /*vtkColor4f*/ newColor4fValue{ 0.1f, 0.2f, 0.3f, 0.4f };
  const nlohmann::json /*vtkColor4ub*/ newColor4ubValue{ 111, 222, 123, 132 };
  const nlohmann::json /*vtkRectd*/ newRectdValue{ 0.0, 1.0, 2.0, 3.0 };
  const nlohmann::json /*vtkRectf*/ newRectfValue{ 0.f, 1.f, 2.f, 3.f };
  const nlohmann::json /*vtkRecti*/ newRectiValue{ 0, 1, 2, 3 };
  const nlohmann::json /*vtkTuple<int, 3>*/ newTupleInt3Value{ 1, 2, 3 };
  const nlohmann::json /*vtkVector<int, 3>*/ newVectorInt3Value{ 4, 5, 6 };
  const nlohmann::json /*vtkVector2d*/ newVector2dValue{ 1.0, 2.0 };
  const nlohmann::json /*vtkVector2f*/ newVector2fValue{ 1.f, 2.f };
  const nlohmann::json /*vtkVector2i*/ newVector2iValue{ 1, 2 };
  const nlohmann::json /*vtkVector3d*/ newVector3dValue{ 1.0, 2.0, 3.0 };
  const nlohmann::json /*vtkVector3f*/ newVector3fValue{ 1.f, 2.f, 3.f };
  const nlohmann::json /*vtkVector3i*/ newVector3iValue{ 1, 2, 3 };
  const nlohmann::json /*vtkVector4d*/ newVector4dValue{ 1.0, 2.0, 3.0, 4.0 };
  const nlohmann::json /*vtkVector4i*/ newVector4iValue{ 1, 2, 3, 4 };

  vtkNew<vtkSerDesMock> target;
  const auto idTargetObject = manager->RegisterObject(target);

  {
    const char* methodName = "CallWithArguments";
    const auto result = manager->Invoke(idTargetObject, methodName,
      nlohmann::json{ newCStyleEnumValue, newMemberScopedEnumValue, newVTKObjectValue,
        newVTKObjectValue, newNumericScalarValue, newNumericArrayValue, newCharPointerValue,
        newStdStringValue, newStdVectorOfIntValue, newStdVectorOfRealValue,
        newStdVectorOfStdStringValue, newBoundingBoxValue, newColor3dValue, newColor3fValue,
        newColor3ubValue, newColor4dValue, newColor4fValue, newColor4ubValue, newRectdValue,
        newRectfValue, newRectiValue, newTupleInt3Value, newVectorInt3Value, newVector2dValue,
        newVector2fValue, newVector2iValue, newVector3dValue, newVector3fValue, newVector3iValue,
        newVector4dValue, newVector4iValue });
  }

  {
    const char* methodName = "ReturnCStyleEnum";
    const auto result = manager->Invoke(idTargetObject, methodName, nlohmann::json::object());
    if (result != newCStyleEnumValue)
    {
      vtkLog(ERROR, << "CStyleEnumValue != newCStyleEnumValue");
      return EXIT_FAILURE;
    }
  }

  {
    const char* methodName = "ReturnMemberScopedEnum";
    const auto result = manager->Invoke(idTargetObject, methodName, nlohmann::json::object());
    if (result != newMemberScopedEnumValue)
    {
      vtkLog(ERROR, << "MemberScopedEnumValue != newMemberScopedEnumValue");
      return EXIT_FAILURE;
    }
  }

  for (const auto& methodName : { "ReturnVTKObjectRawPointer", "ReturnVTKSmartPointer" })
  {
    const auto result = manager->Invoke(idTargetObject, methodName, nlohmann::json::object());
    if (result["Id"].get<vtkTypeUInt32>() != idArgMockObject)
    {
      vtkLogF(ERROR, "Id '%u' is invalid. Expected '%u'", result["Id"].get<vtkTypeUInt32>(),
        idArgMockObject);
      return EXIT_FAILURE;
    }
    const auto tag =
      vtkSerDesMockObject::SafeDownCast(manager->GetObjectAtId(result["Id"]))->GetTag();
    if (tag != 5678)
    {
      vtkLogF(ERROR, "Tag '%u' is invalid. Expected '%d'", tag, 5678);
      return EXIT_FAILURE;
    }
  }

  {
    const char* methodName = "ReturnNumericScalar";
    const auto result = manager->Invoke(idTargetObject, methodName, nlohmann::json::object());
    if (result != newNumericScalarValue)
    {
      vtkLog(ERROR, << "NumericScalarValue != newNumericScalarValue");
      return EXIT_FAILURE;
    }
  }

  {
    const char* methodName = "ReturnNumericArray";
    const auto result = manager->Invoke(idTargetObject, methodName, nlohmann::json::object());
    if (result != newNumericArrayValue)
    {
      vtkLog(ERROR, << "NumericArrayValue != newNumericArrayValue");
      return EXIT_FAILURE;
    }
  }

  {
    const char* methodName = "ReturnCharPointer";
    const auto result = manager->Invoke(idTargetObject, methodName, nlohmann::json::object());
    if (result != newCharPointerValue)
    {
      vtkLog(ERROR, << result.dump());
      vtkLog(ERROR, << "CharPointerValue != newCharPointerValue");
      return EXIT_FAILURE;
    }
  }

  {
    const char* methodName = "ReturnStdString";
    const auto result = manager->Invoke(idTargetObject, methodName, nlohmann::json::object());
    if (result != newStdStringValue)
    {
      vtkLog(ERROR, << "StdStringValue != newStdStringValue");
      return EXIT_FAILURE;
    }
  }

  {
    const char* methodName = "ReturnStdVectorOfInt";
    const auto result = manager->Invoke(idTargetObject, methodName, nlohmann::json::object());
    if (result != newStdVectorOfIntValue)
    {
      vtkLog(ERROR, << "StdVectorOfIntValue != newStdVectorOfIntValue");
      return EXIT_FAILURE;
    }
  }

  {
    const char* methodName = "ReturnStdVectorOfReal";
    const auto result = manager->Invoke(idTargetObject, methodName, nlohmann::json::object());
    if (result != newStdVectorOfRealValue)
    {
      vtkLog(ERROR, << "StdVectorOfRealValue != newStdVectorOfRealValue");
      return EXIT_FAILURE;
    }
  }

  {
    const char* methodName = "ReturnStdVectorOfStdString";
    const auto result = manager->Invoke(idTargetObject, methodName, nlohmann::json::object());
    if (result != newStdVectorOfStdStringValue)
    {
      vtkLog(ERROR, << "StdVectorOfStdStringValue != newStdVectorOfStdStringValue");
      return EXIT_FAILURE;
    }
  }

  {
    const char* methodName = "ReturnBoundingBox";
    const auto result = manager->Invoke(idTargetObject, methodName, nlohmann::json::object());
    if (result != newBoundingBoxValue)
    {
      vtkLog(ERROR, << "BoundingBoxValue != newBoundingBoxValue");
      return EXIT_FAILURE;
    }
  }

  {
    const char* methodName = "ReturnColor3d";
    const auto result = manager->Invoke(idTargetObject, methodName, nlohmann::json::object());
    if (result != newColor3dValue)
    {
      vtkLog(ERROR, << "Color3dValue != newColor3dValue");
      return EXIT_FAILURE;
    }
  }

  {
    const char* methodName = "ReturnColor3f";
    const auto result = manager->Invoke(idTargetObject, methodName, nlohmann::json::object());
    if (result != newColor3fValue)
    {
      vtkLog(ERROR, << "Color3fValue != newColor3fValue");
      return EXIT_FAILURE;
    }
  }

  {
    const char* methodName = "ReturnColor3ub";
    const auto result = manager->Invoke(idTargetObject, methodName, nlohmann::json::object());
    if (result != newColor3ubValue)
    {
      vtkLog(ERROR, << "Color3ubValue != newColor3ubValue");
      return EXIT_FAILURE;
    }
  }

  {
    const char* methodName = "ReturnColor4d";
    const auto result = manager->Invoke(idTargetObject, methodName, nlohmann::json::object());
    if (result != newColor4dValue)
    {
      vtkLog(ERROR, << "Color4dValue != newColor4dValue");
      return EXIT_FAILURE;
    }
  }

  {
    const char* methodName = "ReturnColor4f";
    const auto result = manager->Invoke(idTargetObject, methodName, nlohmann::json::object());
    if (result != newColor4fValue)
    {
      vtkLog(ERROR, << "Color4fValue != newColor4fValue");
      return EXIT_FAILURE;
    }
  }

  {
    const char* methodName = "ReturnColor4ub";
    const auto result = manager->Invoke(idTargetObject, methodName, nlohmann::json::object());
    if (result != newColor4ubValue)
    {
      vtkLog(ERROR, << "Color4ubValue != newColor4ubValue");
      return EXIT_FAILURE;
    }
  }

  {
    const char* methodName = "ReturnRectd";
    const auto result = manager->Invoke(idTargetObject, methodName, nlohmann::json::object());
    if (result != newRectdValue)
    {
      vtkLog(ERROR, << "RectdValue != newRectdValue");
      return EXIT_FAILURE;
    }
  }

  {
    const char* methodName = "ReturnRectf";
    const auto result = manager->Invoke(idTargetObject, methodName, nlohmann::json::object());
    if (result != newRectfValue)
    {
      vtkLog(ERROR, << "RectfValue != newRectfValue");
      return EXIT_FAILURE;
    }
  }

  {
    const char* methodName = "ReturnRecti";
    const auto result = manager->Invoke(idTargetObject, methodName, nlohmann::json::object());
    if (result != newRectiValue)
    {
      vtkLog(ERROR, << "RectiValue != newRectiValue");
      return EXIT_FAILURE;
    }
  }

  {
    const char* methodName = "ReturnTupleInt3";
    const auto result = manager->Invoke(idTargetObject, methodName, nlohmann::json::object());
    if (result != newTupleInt3Value)
    {
      vtkLog(ERROR, << "TupleInt3Value != newTupleInt3Value");
      return EXIT_FAILURE;
    }
  }

  {
    const char* methodName = "ReturnVectorInt3";
    const auto result = manager->Invoke(idTargetObject, methodName, nlohmann::json::object());
    if (result != newVectorInt3Value)
    {
      vtkLog(ERROR, << "VectorInt3Value != newVectorInt3Value");
      return EXIT_FAILURE;
    }
  }

  {
    const char* methodName = "ReturnVector2d";
    const auto result = manager->Invoke(idTargetObject, methodName, nlohmann::json::object());
    if (result != newVector2dValue)
    {
      vtkLog(ERROR, << "Vector2dValue != newVector2dValue");
      return EXIT_FAILURE;
    }
  }

  {
    const char* methodName = "ReturnVector2f";
    const auto result = manager->Invoke(idTargetObject, methodName, nlohmann::json::object());
    if (result != newVector2fValue)
    {
      vtkLog(ERROR, << "Vector2fValue != newVector2fValue");
      return EXIT_FAILURE;
    }
  }

  {
    const char* methodName = "ReturnVector2i";
    const auto result = manager->Invoke(idTargetObject, methodName, nlohmann::json::object());
    if (result != newVector2iValue)
    {
      vtkLog(ERROR, << "Vector2iValue != newVector2iValue");
      return EXIT_FAILURE;
    }
  }

  {
    const char* methodName = "ReturnVector3d";
    const auto result = manager->Invoke(idTargetObject, methodName, nlohmann::json::object());
    if (result != newVector3dValue)
    {
      vtkLog(ERROR, << "Vector3dValue != newVector3dValue");
      return EXIT_FAILURE;
    }
  }

  {
    const char* methodName = "ReturnVector3f";
    const auto result = manager->Invoke(idTargetObject, methodName, nlohmann::json::object());
    if (result != newVector3fValue)
    {
      vtkLog(ERROR, << "Vector3fValue != newVector3fValue");
      return EXIT_FAILURE;
    }
  }

  {
    const char* methodName = "ReturnVector3i";
    const auto result = manager->Invoke(idTargetObject, methodName, nlohmann::json::object());
    if (result != newVector3iValue)
    {
      vtkLog(ERROR, << "Vector3iValue != newVector3iValue");
      return EXIT_FAILURE;
    }
  }

  {
    const char* methodName = "ReturnVector4d";
    const auto result = manager->Invoke(idTargetObject, methodName, nlohmann::json::object());
    if (result != newVector4dValue)
    {
      vtkLog(ERROR, << "Vector4dValue != newVector4dValue");
      return EXIT_FAILURE;
    }
  }

  {
    const char* methodName = "ReturnVector4i";
    const auto result = manager->Invoke(idTargetObject, methodName, nlohmann::json::object());
    if (result != newVector4iValue)
    {
      vtkLog(ERROR, << "Vector4iValue != newVector4iValue");
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
