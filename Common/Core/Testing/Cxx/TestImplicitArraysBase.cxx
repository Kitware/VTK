/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImplicitArraysBase.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImplicitArray.h"

#include "vtkArrayDispatch.h"
#include "vtkDataArrayRange.h"
#include "vtkIntArray.h"

#include <cstdlib>

namespace
{

struct Const42
{
  int operator()(int vtkNotUsed(idx)) const { return 42; };
};

struct ConstStruct
{
  int value = 0.0;

  ConstStruct(int val) { this->value = val; }

  int operator()(int vtkNotUsed(idx)) const { return this->value; };
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

  {
    auto range = vtk::DataArrayValueRange<1>(arr42);
    int iArr = 0;
    for (auto val : range)
    {
      if (val != 42)
      {
        res = EXIT_FAILURE;
        std::cout << iArr << " iterator entry is not equal to constant 42!" << std::endl;
      }
      iArr++;
    }
  }

  {
    vtkNew<vtkIntArray> copied;
    copied->DeepCopy(arr42);
    auto range = vtk::DataArrayValueRange<1>(copied);
    int iArr = 0;
    for (auto val : range)
    {
      if (val != 42)
      {
        res = EXIT_FAILURE;
        std::cout << iArr << " deep copied entry is not equal to constant 42!" << std::endl;
      }
      iArr++;
    }
  }

  {
    vtkNew<vtkIntArray> copied;
    copied->ShallowCopy(arr42);
    auto range = vtk::DataArrayValueRange<1>(copied);
    int iArr = 0;
    for (auto val : range)
    {
      if (val != 42)
      {
        res = EXIT_FAILURE;
        std::cout << iArr << " shallow copied entry is not equal to constant 42!" << std::endl;
      }
      iArr++;
    }
  }

  {
    vtkNew<vtkImplicitArray<::Const42>> copied;
    copied->ImplicitDeepCopy(arr42.Get());
    auto range = vtk::DataArrayValueRange<1>(copied);
    int iArr = 0;
    for (auto val : range)
    {
      if (val != 42)
      {
        res = EXIT_FAILURE;
        std::cout << iArr << " deep copied implicit array entry is not equal to constant 42!"
                  << std::endl;
      }
      iArr++;
    }
  }

  /*
   * Fails to compile, as it should, since the backend types are not the same
   */
  //{
  // vtkNew<vtkStdFunctionArray<int>> copied;
  // copied->DeepCopy(arr42.Get());
  //}

  {
    int* vPtr = static_cast<int*>(arr42->GetVoidPointer(0));
    for (int iArr = 0; iArr < 100; iArr++)
    {
      if (vPtr[iArr] != 42)
      {
        res = EXIT_FAILURE;
        std::cout << iArr << " void pointer entry is not equal to constant 42!" << std::endl;
      }
    }
    arr42->Squeeze();
  }

  {
    vtkNew<vtkImplicitArray<::ConstStruct>> genericConstArr;
    genericConstArr->SetBackend(std::make_shared<::ConstStruct>(42));
    genericConstArr->SetNumberOfComponents(2);
    genericConstArr->SetNumberOfTuples(50);
    for (int iArr = 0; iArr < 50; iArr++)
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
  }

  return res;
}
