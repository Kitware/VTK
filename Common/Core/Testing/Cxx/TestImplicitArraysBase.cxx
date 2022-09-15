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

#include "vtkDataArrayRange.h"

#include <cstdlib>

namespace
{

struct Const42
{
  int operator()(int idx) const { return 42; };
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

  //{
  // auto range = vtk::DataArrayValueRange<1>(arr42);
  // int iArr = 0;
  // for(auto val : range)
  //{
  // std::cout << iArr << std::endl;
  // std::cout << val <<  std::endl;
  // if(val != 42)
  //{
  // res = EXIT_FAILURE;
  // std::cout << iArr << "iterator entry is not equal to constant 42!" << std::endl;
  //}
  // iArr++;
  //}
  //}

  return res;
}
