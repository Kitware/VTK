// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkSerDesMock
 * @brief   A mock interface for testing the SerDes infrastructure
 *
 * Provides all properties supported by SerDes and member functions that
 * can be invoked by the `vtkInvoker`.
 */

#ifndef vtkSerDesMock_h
#define vtkSerDesMock_h

#include "vtkObject.h"

#include "vtkBoundingBox.h"                // for vtkBoundingBox
#include "vtkColor.h"                      // for vtkColor
#include "vtkRect.h"                       // for vtkRect
#include "vtkSerDesMockObject.h"           // for vtkSerDesMockObject
#include "vtkSmartPointer.h"               // for vtkSmartPointer
#include "vtkTestingSerializationModule.h" // For export macro
#include "vtkTuple.h"                      // for vtkTuple
#include "vtkVector.h"                     // for vtkVector
#include <string>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

class VTKTESTINGSERIALIZATION_EXPORT VTK_MARSHALAUTO vtkSerDesMock : public vtkObject
{
public:
  /**
   * Standard object factory instantiation method.
   */
  static vtkSerDesMock* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkTypeMacro(vtkSerDesMock, vtkObject);

  enum CStyleEnum
  {
    Value1,
    Value2
  };
  enum class MemberScopedEnum
  {
    Value1,
    Value2
  };

  void CallWithArguments(CStyleEnum arg0, MemberScopedEnum arg1, vtkSerDesMockObject* arg2,
    vtkSmartPointer<vtkSerDesMockObject> arg3, double arg4, float arg5[4], char* arg6,
    const std::string& arg7, const std::vector<int>& arg8, const std::vector<float>& arg9,
    const std::vector<std::string>& arg10, const std::vector<vtkSerDesMockObject*>& arg11,
    const vtkBoundingBox& arg12, const vtkColor3d& arg13, const vtkColor3f& arg14,
    const vtkColor3ub& arg15, const vtkColor4d& arg16, const vtkColor4f& arg17,
    const vtkColor4ub& arg18, const vtkRectd& arg19, const vtkRectf& arg20, const vtkRecti& arg21,
    const vtkTuple<int, 3>& arg22, const vtkVector<int, 3>& arg23, const vtkVector2d& arg24,
    const vtkVector2f& arg25, const vtkVector2i& arg26, const vtkVector3d& arg27,
    const vtkVector3f& arg28, const vtkVector3i& arg29, const vtkVector4d& arg30,
    const vtkVector4i& arg31);

  CStyleEnum ReturnCStyleEnum() { return this->CStyleEnumValue; }

  MemberScopedEnum ReturnMemberScopedEnum() { return this->MemberScopedEnumValue; }

  vtkSerDesMockObject* ReturnVTKObjectRawPointer() { return this->ObjectRawPointerValue; };

  vtkSmartPointer<vtkSerDesMockObject> ReturnVTKSmartPointer()
  {
    return this->ObjectSmartPointerValue;
  };

  double ReturnNumericScalar() { return this->NumericScalarValue; }

  float* ReturnNumericArray() VTK_SIZEHINT(4) { return this->NumericArrayValue; }

  const char* ReturnCharPointer() { return this->CharPointerValue; }

  std::string ReturnStdString() { return this->StdStringValue; }

  std::vector<int> ReturnStdVectorOfInt() { return this->StdVectorOfIntValue; }

  std::vector<float> ReturnStdVectorOfReal() { return this->StdVectorOfRealValue; }

  std::vector<std::string> ReturnStdVectorOfStdString() { return this->StdVectorOfStdStringValue; }

  std::vector<vtkSerDesMockObject*> ReturnStdVectorOfVTKObjectRawPointer()
  {
    return this->StdVectorOfVTKObjectRawPointerValue;
  }

  vtkBoundingBox ReturnBoundingBox() { return this->BoundingBoxValue; }

  vtkColor3d ReturnColor3d() { return this->Color3dValue; }

  vtkColor3f ReturnColor3f() { return this->Color3fValue; }

  vtkColor3ub ReturnColor3ub() { return this->Color3ubValue; }

  vtkColor4d ReturnColor4d() { return this->Color4dValue; }

  vtkColor4f ReturnColor4f() { return this->Color4fValue; }

  vtkColor4ub ReturnColor4ub() { return this->Color4ubValue; }

  vtkRectd ReturnRectd() { return this->RectdValue; }

  vtkRectf ReturnRectf() { return this->RectfValue; }

  vtkRecti ReturnRecti() { return this->RectiValue; }

  vtkTuple<int, 3> ReturnTupleInt3() { return this->TupleInt3Value; }

  vtkVector<int, 3> ReturnVectorInt3() { return this->VectorInt3Value; }

  vtkVector2d ReturnVector2d() { return this->Vector2dValue; }

  vtkVector2f ReturnVector2f() { return this->Vector2fValue; }

  vtkVector2i ReturnVector2i() { return this->Vector2iValue; }

  vtkVector3d ReturnVector3d() { return this->Vector3dValue; }

  vtkVector3f ReturnVector3f() { return this->Vector3fValue; }

  vtkVector3i ReturnVector3i() { return this->Vector3iValue; }

  vtkVector4d ReturnVector4d() { return this->Vector4dValue; }

  vtkVector4i ReturnVector4i() { return this->Vector4iValue; }

protected:
  vtkSerDesMock();
  ~vtkSerDesMock() override;

private:
  vtkSerDesMock(const vtkSerDesMock&) = delete;
  void operator=(const vtkSerDesMock&) = delete;

  CStyleEnum CStyleEnumValue = Value1;
  MemberScopedEnum MemberScopedEnumValue = MemberScopedEnum::Value1;
  vtkSerDesMockObject* ObjectRawPointerValue = nullptr;
  vtkSmartPointer<vtkSerDesMockObject> ObjectSmartPointerValue;
  double NumericScalarValue = 0;
  float NumericArrayValue[4] = { 0.f, 0.f, 0.f, 0.f };
  char* CharPointerValue = nullptr;
  std::string StdStringValue;
  std::vector<int> StdVectorOfIntValue;
  std::vector<float> StdVectorOfRealValue;
  std::vector<std::string> StdVectorOfStdStringValue;
  std::vector<vtkSerDesMockObject*> StdVectorOfVTKObjectRawPointerValue;
  vtkBoundingBox BoundingBoxValue;
  vtkColor3d Color3dValue;
  vtkColor3f Color3fValue;
  vtkColor3ub Color3ubValue;
  vtkColor4d Color4dValue;
  vtkColor4f Color4fValue;
  vtkColor4ub Color4ubValue;
  vtkRectd RectdValue;
  vtkRectf RectfValue;
  vtkRecti RectiValue;
  vtkTuple<int, 3> TupleInt3Value;
  vtkVector<int, 3> VectorInt3Value;
  vtkVector2d Vector2dValue;
  vtkVector2f Vector2fValue;
  vtkVector2i Vector2iValue;
  vtkVector3d Vector3dValue;
  vtkVector3f Vector3fValue;
  vtkVector3i Vector3iValue;
  vtkVector4d Vector4dValue;
  vtkVector4i Vector4iValue;
};
VTK_ABI_NAMESPACE_END
#endif
