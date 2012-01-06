/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ArrayUserTypes.cxx

-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkDenseArray.h>
#include <vtkSmartPointer.h>
#include <vtkSparseArray.h>

#include <vtksys/ios/iostream>
#include <vtksys/ios/sstream>
#include <vtksys/stl/stdexcept>

#define test_expression(expression) \
{ \
  if(!(expression)) \
    { \
    vtksys_ios::ostringstream buffer; \
    buffer << "Expression failed at line " << __LINE__ << ": " << #expression; \
    throw std::runtime_error(buffer.str()); \
    } \
}

class UserType
{
public:
  UserType() :
    Value("")
  {
  }

  UserType(const vtkStdString& value) :
    Value(value)
  {
  }

  bool operator==(const UserType& other) const
  {
    return this->Value == other.Value;
  }

  vtkStdString Value;
};

template<>
inline UserType vtkVariantCast<UserType>(const vtkVariant& value, bool* valid)
{
  if(valid)
    *valid = true;
  return UserType(value.ToString());
}

template<>
inline vtkVariant vtkVariantCreate<UserType>(const UserType& value)
{
  return vtkVariant(value.Value);
}

int TestArrayUserTypes(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  try
    {
    vtkSmartPointer<vtkDenseArray<UserType> > dense = vtkSmartPointer<vtkDenseArray<UserType> >::New();
    dense->Resize(3, 4);
    dense->Fill(UserType("red"));
    for(vtkArray::SizeT n = 0; n != dense->GetNonNullSize(); ++n)
      {
      test_expression(dense->GetValueN(n) == UserType("red"));
      }

    dense->SetValue(1, 2, UserType("green"));
    test_expression(dense->GetValue(1, 2) == UserType("green"));

    dense->SetVariantValue(1, 2, vtkVariant("puce"));
    test_expression(dense->GetValue(1, 2) == UserType("puce"));
    test_expression(dense->GetVariantValue(1, 2) == vtkVariant("puce"));

    vtkSmartPointer<vtkSparseArray<UserType> > sparse = vtkSmartPointer<vtkSparseArray<UserType> >::New();
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
  catch(std::exception& e)
    {
    cerr << e.what() << endl;
    return 1;
    }
}
