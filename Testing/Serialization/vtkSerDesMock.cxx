// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSerDesMock.h"
#include "vtkObjectFactory.h"
#include "vtkSerDesMockObject.h"

#include <algorithm>
#include <iterator>
#include <type_traits>

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkSerDesMock);

vtkSerDesMock::vtkSerDesMock() = default;

vtkSerDesMock::~vtkSerDesMock()
{
  if (this->ObjectRawPointerValue)
  {
    this->ObjectRawPointerValue->UnRegister(this);
    this->ObjectRawPointerValue = nullptr;
  }
}

void vtkSerDesMock::PrintSelf(ostream& os, vtkIndent indent)
{
  os << "CStyleEnumValue: " << this->CStyleEnumValue << '\n';
  os << "MemberScopedEnumValue: "
     << static_cast<std::underlying_type_t<MemberScopedEnum>>(this->MemberScopedEnumValue) << '\n';
  os << "ObjectRawPointerValue: " << this->ObjectRawPointerValue << '\n';
  os << "ObjectSmartPointerValue: " << this->ObjectSmartPointerValue << '\n';
  os << "NumericScalarValue: " << this->NumericScalarValue << '\n';
  os << "NumericArrayValue: ";
  std::copy(this->NumericArrayValue,
    this->NumericArrayValue +
      (sizeof(this->NumericArrayValue) / sizeof(this->NumericArrayValue[0])),
    std::ostream_iterator<float>(os, ","));
  os << '\n';
  os << "CharPointerValue: " << this->CharPointerValue << '\n';
  os << "StdStringValue: " << this->StdStringValue << '\n';
  os << "StdVectorOfIntValue: ";
  std::copy(this->StdVectorOfIntValue.begin(), this->StdVectorOfIntValue.end(),
    std::ostream_iterator<int>(os, ","));
  os << '\n';
  os << "StdVectorOfRealValue: ";
  std::copy(this->StdVectorOfRealValue.begin(), this->StdVectorOfRealValue.end(),
    std::ostream_iterator<float>(os, ","));
  os << '\n';
  os << "StdVectorOfStdStringValue: ";
  std::copy(this->StdVectorOfStdStringValue.begin(), this->StdVectorOfStdStringValue.end(),
    std::ostream_iterator<std::string>(os, ","));
  os << '\n';
  os << "BoundingBoxValue: "
     << "xMin=" << this->BoundingBoxValue.GetBound(0)
     << "xMax=" << this->BoundingBoxValue.GetBound(1)
     << "yMin=" << this->BoundingBoxValue.GetBound(2)
     << "yMax=" << this->BoundingBoxValue.GetBound(3)
     << "zMin=" << this->BoundingBoxValue.GetBound(4)
     << "zMax=" << this->BoundingBoxValue.GetBound(5) << '\n';
  os << "Color3dValue: " << this->Color3dValue << '\n';
  os << "Color3fValue: " << this->Color3fValue << '\n';
  os << "Color3ubValue: " << this->Color3ubValue << '\n';
  os << "Color4dValue: " << this->Color4dValue << '\n';
  os << "Color4fValue: " << this->Color4fValue << '\n';
  os << "Color4ubValue: " << this->Color4ubValue << '\n';
  os << "RectdValue: " << this->RectdValue << '\n';
  os << "RectfValue: " << this->RectfValue << '\n';
  os << "RectiValue: " << this->RectiValue << '\n';
  os << "TupleInt3Value: " << this->TupleInt3Value << '\n';
  os << "VectorInt3Value: " << this->VectorInt3Value << '\n';
  os << "Vector2dValue: " << this->Vector2dValue << '\n';
  os << "Vector2fValue: " << this->Vector2fValue << '\n';
  os << "Vector2iValue: " << this->Vector2iValue << '\n';
  os << "Vector3dValue: " << this->Vector3dValue << '\n';
  os << "Vector3fValue: " << this->Vector3fValue << '\n';
  os << "Vector3iValue: " << this->Vector3iValue << '\n';
  os << "Vector4dValue: " << this->Vector4dValue << '\n';
  os << "Vector4iValue: " << this->Vector4iValue << '\n';
  this->Superclass::PrintSelf(os, indent);
}

void vtkSerDesMock::CallWithArguments(CStyleEnum arg0, MemberScopedEnum arg1,
  vtkSerDesMockObject* arg2, vtkSmartPointer<vtkSerDesMockObject> arg3, double arg4, float arg5[4],
  char* arg6, const std::string& arg7, const std::vector<int>& arg8, const std::vector<float>& arg9,
  const std::vector<std::string>& arg10, const vtkBoundingBox& arg11, const vtkColor3d& arg12,
  const vtkColor3f& arg13, const vtkColor3ub& arg14, const vtkColor4d& arg15,
  const vtkColor4f& arg16, const vtkColor4ub& arg17, const vtkRectd& arg18, const vtkRectf& arg19,
  const vtkRecti& arg20, const vtkTuple<int, 3>& arg21, const vtkVector<int, 3>& arg22,
  const vtkVector2d& arg23, const vtkVector2f& arg24, const vtkVector2i& arg25,
  const vtkVector3d& arg26, const vtkVector3f& arg27, const vtkVector3i& arg28,
  const vtkVector4d& arg29, const vtkVector4i& arg30)
{
  this->CStyleEnumValue = arg0;
  this->MemberScopedEnumValue = arg1;
  vtkSetObjectBodyMacro(ObjectRawPointerValue, vtkSerDesMockObject, arg2);
  vtkSetSmartPointerBodyMacro(ObjectSmartPointerValue, vtkSerDesMockObject, arg3);
  this->NumericScalarValue = arg4;
  std::copy(arg5, arg5 + (sizeof(this->NumericArrayValue) / sizeof(this->NumericArrayValue[0])),
    this->NumericArrayValue);
  vtkSetStringBodyMacro(CharPointerValue, arg6);
  this->StdStringValue = arg7;
  std::copy(arg8.begin(), arg8.end(), std::back_inserter(this->StdVectorOfIntValue));
  std::copy(arg9.begin(), arg9.end(), std::back_inserter(this->StdVectorOfRealValue));
  std::copy(arg10.begin(), arg10.end(), std::back_inserter(this->StdVectorOfStdStringValue));
  this->BoundingBoxValue = arg11;
  this->Color3dValue = arg12;
  this->Color3fValue = arg13;
  this->Color3ubValue = arg14;
  this->Color4dValue = arg15;
  this->Color4fValue = arg16;
  this->Color4ubValue = arg17;
  this->RectdValue = arg18;
  this->RectfValue = arg19;
  this->RectiValue = arg20;
  this->TupleInt3Value = arg21;
  this->VectorInt3Value = arg22;
  this->Vector2dValue = arg23;
  this->Vector2fValue = arg24;
  this->Vector2iValue = arg25;
  this->Vector3dValue = arg26;
  this->Vector3fValue = arg27;
  this->Vector3iValue = arg28;
  this->Vector4dValue = arg29;
  this->Vector4iValue = arg30;
}
VTK_ABI_NAMESPACE_END
