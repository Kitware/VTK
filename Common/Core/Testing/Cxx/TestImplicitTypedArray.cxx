// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAffineTypeInt32Array.h"
#include "vtkCompositeTypeInt32Array.h"
#include "vtkConstantTypeInt32Array.h"
#include "vtkIndexedTypeInt32Array.h"

#include "vtkNew.h"

#include <iostream>

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
  vtkNew<vtkConstantTypeInt32Array> constInt;
  constInt->SetNumberOfTuples(100);
  constInt->ConstructBackend(42);

  if (!CheckNewInstance(constInt, vtkConstantTypeInt32Array::ArrayTypeTag::value))
  {
    std::cout << "Failed with vtkConstantTypeInt32Array\n";
    return EXIT_FAILURE;
  }

  vtkNew<vtkAffineTypeInt32Array> affine;
  if (!CheckNewInstance(affine, vtkAffineTypeInt32Array::ArrayTypeTag::value))
  {
    std::cout << "Failed with vtkAffineTypeInt32Array\n";
    return EXIT_FAILURE;
  }

  vtkNew<vtkCompositeTypeInt32Array> composite;
  if (!CheckNewInstance(composite, vtkCompositeTypeInt32Array::ArrayTypeTag::value))
  {
    std::cout << "Failed with vtkCompositeTypeInt32Array\n";
    return EXIT_FAILURE;
  }

  vtkNew<vtkIndexedTypeInt32Array> indexed;
  if (!CheckNewInstance(indexed, vtkIndexedTypeInt32Array::ArrayTypeTag::value))
  {
    std::cout << "Failed with vtkIndexedTypeInt32Array\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
