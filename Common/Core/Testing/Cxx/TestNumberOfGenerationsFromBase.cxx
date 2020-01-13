/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestVariant.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <vtkFloatArray.h>
#include <vtkIntArray.h>
#include <vtkLogger.h>
#include <vtkNew.h>
#include <vtkObject.h>

int TestNumberOfGenerationsFromBase(int, char*[])
{
  vtkNew<vtkFloatArray> floatArray;
  if (floatArray->GetNumberOfGenerationsFromBase(vtkNew<vtkObject>()->GetClassName()) != 5)
  {
    vtkLog(ERROR,
      "Incorrect number of generations between "
        << floatArray->GetClassName() << " and " << vtkNew<vtkObject>()->GetClassName()
        << " (received "
        << floatArray->GetNumberOfGenerationsFromBase(vtkNew<vtkObject>()->GetClassName())
        << ", should be " << 5);
    return EXIT_FAILURE;
  }

  if (floatArray->GetNumberOfGenerationsFromBase("vtkAbstractArray") != 4)
  {
    vtkLog(ERROR,
      "Incorrect number of generations between "
        << floatArray->GetClassName() << " and "
        << "vtkAbstractArray"
        << " (received " << floatArray->GetNumberOfGenerationsFromBase("vtkAbstractArray")
        << ", should be " << 4);
    return EXIT_FAILURE;
  }

  if (floatArray->GetNumberOfGenerationsFromBase(floatArray->GetClassName()) != 0)
  {
    vtkLog(ERROR,
      "Incorrect number of generations between "
        << floatArray->GetClassName() << " and " << floatArray->GetClassName() << " (received "
        << floatArray->GetNumberOfGenerationsFromBase(vtkNew<vtkObject>()->GetClassName())
        << ", should be " << 0);
    return EXIT_FAILURE;
  }

  vtkNew<vtkIntArray> intArray;
  if (floatArray->GetNumberOfGenerationsFromBase(intArray->GetClassName()) >= 0)
  {
    vtkLog(ERROR,
      "Incorrect number of generations between "
        << floatArray->GetClassName() << " and " << intArray->GetClassName() << " (received "
        << floatArray->GetNumberOfGenerationsFromBase(intArray->GetClassName())
        << ", should be < 0");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
