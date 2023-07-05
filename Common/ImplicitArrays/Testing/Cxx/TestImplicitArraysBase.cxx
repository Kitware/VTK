// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkImplicitArray.h"

#include "vtkArrayDispatch.h"
#include "vtkDataArrayRange.h"
#include "vtkIntArray.h"

#include <cstdlib>

namespace
{

struct Const42
{
  int operator()(int vtkNotUsed(idx)) const { return 42; }
};

struct ConstStruct
{
  int value = 0.0;

  ConstStruct(int val) { this->value = val; }

  int operator()(int vtkNotUsed(idx)) const { return this->value; }
};

struct ConstTupleStruct
{
  int Tuple[3] = { 0, 0, 0 };

  ConstTupleStruct(int tuple[3])
  {
    this->Tuple[0] = tuple[0];
    this->Tuple[1] = tuple[1];
    this->Tuple[2] = tuple[2];
  }

  // used for GetValue
  int map(int idx) const
  {
    int tuple[3];
    this->mapTuple(idx / 3, tuple);
    return tuple[idx % 3];
  }
  // used for GetTypedTuple
  void mapTuple(int vtkNotUsed(idx), int* tuple) const
  {
    tuple[0] = this->Tuple[0];
    tuple[1] = this->Tuple[1];
    tuple[2] = this->Tuple[2];
  }
};

struct ConstComponentStruct
{
  int Tuple[3] = { 0, 0, 0 };

  ConstComponentStruct(int tuple[3])
  {
    this->Tuple[0] = tuple[0];
    this->Tuple[1] = tuple[1];
    this->Tuple[2] = tuple[2];
  }

  // used for GetValue
  int map(int idx) const { return this->mapComponent(idx / 3, idx % 3); }
  // used for GetTypedComponent
  int mapComponent(int vtkNotUsed(idx), int comp) const { return this->Tuple[comp]; }
};

};

int TestImplicitArraysBase(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  int res = EXIT_SUCCESS;

  vtkNew<vtkImplicitArray<::Const42>> arr42;
  arr42->SetNumberOfComponents(1);
  arr42->SetNumberOfTuples(100);

  if (arr42->GetNumberOfComponents() != 1)
  {
    res = EXIT_FAILURE;
    std::cout << "Number of components did not set properly" << std::endl;
  }

  if (arr42->GetNumberOfTuples() != 100)
  {
    res = EXIT_FAILURE;
    std::cout << "Number of tuples did not set properly" << std::endl;
  }

  for (int iArr = 0; iArr < 100; iArr++)
  {
    if (arr42->GetValue(iArr) != 42)
    {
      res = EXIT_FAILURE;
      std::cout << iArr << "th entry is not equal to constant 42!" << std::endl;
    }
  }

  auto baseRange = vtk::DataArrayValueRange<1>(arr42);
  int iArr = 0;
  for (auto val : baseRange)
  {
    if (val != 42)
    {
      res = EXIT_FAILURE;
      std::cout << iArr << " iterator entry is not equal to constant 42!" << std::endl;
    }
    iArr++;
  }

  vtkNew<vtkIntArray> copied;
  copied->DeepCopy(arr42);
  auto copiedRange = vtk::DataArrayValueRange<1>(copied);
  iArr = 0;
  for (auto val : copiedRange)
  {
    if (val != 42)
    {
      res = EXIT_FAILURE;
      std::cout << iArr << " deep copied entry is not equal to constant 42!" << std::endl;
    }
    iArr++;
  }

  vtkNew<vtkIntArray> shopied;
  shopied->ShallowCopy(arr42);
  auto shopiedRange = vtk::DataArrayValueRange<1>(shopied);
  iArr = 0;
  for (auto val : shopiedRange)
  {
    if (val != 42)
    {
      res = EXIT_FAILURE;
      std::cout << iArr << " shallow copied entry is not equal to constant 42!" << std::endl;
    }
    iArr++;
  }

  vtkNew<vtkImplicitArray<::Const42>> impCopied;
  impCopied->ImplicitDeepCopy(arr42.Get());
  auto impCopiedRange = vtk::DataArrayValueRange<1>(impCopied);
  iArr = 0;
  for (auto val : impCopiedRange)
  {
    if (val != 42)
    {
      res = EXIT_FAILURE;
      std::cout << iArr << " deep copied implicit array entry is not equal to constant 42!"
                << std::endl;
    }
    iArr++;
  }

  int* vPtr = static_cast<int*>(arr42->GetVoidPointer(0));
  for (iArr = 0; iArr < 100; iArr++)
  {
    if (vPtr[iArr] != 42)
    {
      res = EXIT_FAILURE;
      std::cout << iArr << " void pointer entry is not equal to constant 42!" << std::endl;
    }
  }
  arr42->Squeeze();

  vtkSmartPointer<vtkDataArray> newInstance;
  newInstance.TakeReference(arr42->NewInstance());
  if (!vtkAOSDataArrayTemplate<int>::SafeDownCast(newInstance))
  {
    res = EXIT_FAILURE;
    std::cout << "NewInstance did not return the correct AOS type array." << std::endl;
  }

  vtkNew<vtkImplicitArray<::ConstStruct>> genericConstArr;
  genericConstArr->ConstructBackend(42);
  genericConstArr->SetNumberOfComponents(2);
  genericConstArr->SetNumberOfTuples(50);
  for (iArr = 0; iArr < 50; iArr++)
  {
    for (int iComp = 0; iComp < 2; iComp++)
    {
      if (genericConstArr->GetComponent(iArr, iComp) != 42)
      {
        res = EXIT_FAILURE;
        std::cout << iArr << " generic ConstStruct component entry is not equal to constant 42!"
                  << std::endl;
      }
    }
  }

  // test backend with mapTuple
  vtkNew<vtkImplicitArray<::ConstTupleStruct>> genericTupleConstArr;
  int tuple[3] = { 1, 2, 3 };
  genericTupleConstArr->ConstructBackend(tuple);
  genericTupleConstArr->SetNumberOfComponents(3);
  genericTupleConstArr->SetNumberOfTuples(50);

  // test GetValue
  for (iArr = 0; iArr < 150; iArr += 3)
  {
    if (genericTupleConstArr->GetValue(iArr + 0) != 1 ||
      genericTupleConstArr->GetValue(iArr + 1) != 2 ||
      genericTupleConstArr->GetValue(iArr + 2) != 3)
    {
      res = EXIT_FAILURE;
      std::cout << iArr << " generic ConstTupleStruct tuple entry is not equal to constant 1, 2, 3!"
                << std::endl;
    }
  }

  // test GetTypedTuple
  for (iArr = 0; iArr < 50; iArr++)
  {
    genericTupleConstArr->GetTypedTuple(iArr, tuple);
    if (tuple[0] != 1 || tuple[1] != 2 || tuple[2] != 3)
    {
      res = EXIT_FAILURE;
      std::cout << iArr << " generic ConstTupleStruct tuple entry is not equal to constant 1, 2, 3!"
                << std::endl;
    }
  }

  // test backend with mapComponent
  vtkNew<vtkImplicitArray<::ConstComponentStruct>> genericComponentConstArr;
  genericComponentConstArr->ConstructBackend(tuple);
  genericComponentConstArr->SetNumberOfComponents(3);
  genericComponentConstArr->SetNumberOfTuples(50);

  // test GetValue
  for (iArr = 0; iArr < 150; iArr += 3)
  {
    if (genericComponentConstArr->GetValue(iArr + 0) != 1 ||
      genericComponentConstArr->GetValue(iArr + 1) != 2 ||
      genericComponentConstArr->GetValue(iArr + 2) != 3)
    {
      res = EXIT_FAILURE;
      std::cout << iArr
                << " generic ConstComponentStruct component entry is not equal to constant 1, 2, 3!"
                << std::endl;
    }
  }

  // test GetTypedComponent
  for (iArr = 0; iArr < 150; iArr++)
  {
    if (genericComponentConstArr->GetTypedComponent(iArr, 0) != 1 ||
      genericComponentConstArr->GetTypedComponent(iArr, 1) != 2 ||
      genericComponentConstArr->GetTypedComponent(iArr, 2) != 3)
    {
      res = EXIT_FAILURE;
      std::cout << iArr
                << " generic ConstComponentStruct component entry is not equal to constant 1, 2, 3!"
                << std::endl;
    }
  }

  return res;
}
