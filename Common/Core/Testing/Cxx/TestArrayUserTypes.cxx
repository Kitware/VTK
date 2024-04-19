// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include <vtkDenseArray.h>
#include <vtkSmartPointer.h>
#include <vtkSparseArray.h>

#include <iostream>
#include <sstream>
#include <stdexcept>

#define test_expression(expression)                                                                \
  do                                                                                               \
  {                                                                                                \
    if (!(expression))                                                                             \
    {                                                                                              \
      std::ostringstream buffer;                                                                   \
      buffer << "Expression failed at line " << __LINE__ << ": " << #expression;                   \
      throw std::runtime_error(buffer.str());                                                      \
    }                                                                                              \
  } while (false)

class UserType
{
public:
  UserType() = default;

  UserType(const std::string& value)
    : Value(value)
  {
  }

  bool operator==(const UserType& other) const { return this->Value == other.Value; }

  std::string Value;
};

VTK_ABI_NAMESPACE_BEGIN

template <>
inline UserType vtkVariantCast<UserType>(const vtkVariant& value, bool* valid)
{
  if (valid)
    *valid = true;
  return UserType(value.ToString());
}

template <>
inline vtkVariant vtkVariantCreate<UserType>(const UserType& value)
{
  return vtkVariant(value.Value);
}
VTK_ABI_NAMESPACE_END

int TestArrayUserTypes(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  try
  {
    vtkSmartPointer<vtkDenseArray<UserType>> dense =
      vtkSmartPointer<vtkDenseArray<UserType>>::New();
    dense->Resize(3, 4);
    dense->Fill(UserType("red"));
    for (vtkArray::SizeT n = 0; n != dense->GetNonNullSize(); ++n)
    {
      test_expression(dense->GetValueN(n) == UserType("red"));
    }

    dense->SetValue(1, 2, UserType("green"));
    test_expression(dense->GetValue(1, 2) == UserType("green"));

    dense->SetVariantValue(1, 2, vtkVariant("puce"));
    test_expression(dense->GetValue(1, 2) == UserType("puce"));
    test_expression(dense->GetVariantValue(1, 2) == vtkVariant("puce"));

    vtkSmartPointer<vtkSparseArray<UserType>> sparse =
      vtkSmartPointer<vtkSparseArray<UserType>>::New();
    sparse->Resize(3, 4);
    sparse->SetNullValue(UserType("blue"));
    test_expression(sparse->GetNullValue() == UserType("blue"));
    test_expression(sparse->GetValue(1, 2) == UserType("blue"));

    sparse->SetValue(0, 1, UserType("white"));
    test_expression(sparse->GetValue(0, 1) == UserType("white"));

    sparse->AddValue(2, 3, UserType("yellow"));
    test_expression(sparse->GetValue(2, 3) == UserType("yellow"));

    sparse->SetVariantValue(2, 3, vtkVariant("slate"));
    test_expression(sparse->GetValue(2, 3) == UserType("slate"));
    test_expression(sparse->GetVariantValue(2, 3) == vtkVariant("slate"));

    return 0;
  }
  catch (std::exception& e)
  {
    cerr << e.what() << endl;
    return 1;
  }
}
