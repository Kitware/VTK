// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAffineIntArray.h"
#include "vtkCompositeIntArray.h"
#include "vtkConstantIntArray.h"
#include "vtkIndexedIntArray.h"

#include "vtkNew.h"

//-------------------------------------------------------------------------------------------------
bool CheckNewInstance(vtkDataArray* sourceArray, int arrayType)
{
  if (sourceArray->GetArrayType() != arrayType)
  {
    std::cout << "GetArrayType did not return " << sourceArray->GetArrayTypeAsString(arrayType)
              << ".\n";

    return false;
  }

  vtkSmartPointer<vtkDataArray> newInstance;
  newInstance.TakeReference(sourceArray->NewInstance());

  if (!vtkAOSDataArrayTemplate<int>::SafeDownCast(newInstance))
  {
    std::cout << "NewInstance did not return the correct AOS type array.\n";
    return false;
  }

  return true;
}

//-------------------------------------------------------------------------------------------------
int TestImplicitTypedArray(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkNew<vtkConstantIntArray> constInt;
  constInt->SetNumberOfTuples(100);
  constInt->ConstructBackend(42);

  if (!CheckNewInstance(constInt, vtkConstantIntArray::ArrayTypeTag::value))
  {
    std::cout << "Failed with vtkConstantIntArray\n";
    return EXIT_FAILURE;
  }

  vtkNew<vtkAffineIntArray> affine;
  if (!CheckNewInstance(affine, vtkAffineIntArray::ArrayTypeTag::value))
  {
    std::cout << "Failed with vtkAffineIntArray\n";
    return EXIT_FAILURE;
  }

  vtkNew<vtkCompositeIntArray> composite;
  if (!CheckNewInstance(composite, vtkCompositeIntArray::ArrayTypeTag::value))
  {
    std::cout << "Failed with vtkCompositeIntArray\n";
    return EXIT_FAILURE;
  }

  vtkNew<vtkIndexedIntArray> indexed;
  if (!CheckNewInstance(indexed, vtkIndexedIntArray::ArrayTypeTag::value))
  {
    std::cout << "Failed with vtkIndexedIntArray\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
