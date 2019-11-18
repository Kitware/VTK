/*==============================================================================

  Program:   Visualization Toolkit
  Module:    TestScaledSOADataArrayTemplate.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

==============================================================================*/
#include "vtkMathUtilities.h"
#include "vtkScaledSOADataArrayTemplate.h"

// Needed for portable setenv on MSVC...
#include "vtksys/SystemTools.hxx"

int TestScaledSOADataArrayTemplate(int, char*[])
{
  const int numValues = 5;
  const double trueFirstData[numValues] = { 0, 1, 2, 3, 4 };
  const double trueSecondData[numValues] = { 10, 11, 12, 13, 14 };
  double firstData[numValues];
  double secondData[numValues];
  for (int i = 0; i < 5; i++)
  {
    firstData[i] = trueFirstData[i];
    secondData[i] = trueSecondData[i];
  }

  vtkScaledSOADataArrayTemplate<double>* array = vtkScaledSOADataArrayTemplate<double>::New();
  array->SetNumberOfComponents(2);
  array->SetNumberOfTuples(numValues);
  array->SetArray(0, firstData, numValues, false, true);
  array->SetArray(1, secondData, numValues, false, true);
  array->SetScale(2.);

  // first check that we get twice the values that are stored in firstData and secondData
  // returned by GetTypedTuple()
  double vals[2];
  for (vtkIdType i = 0; i < array->GetNumberOfTuples(); i++)
  {
    array->GetTypedTuple(i, vals);
    if (!vtkMathUtilities::NearlyEqual(vals[0], trueFirstData[i] * array->GetScale()) ||
      !vtkMathUtilities::NearlyEqual(vals[1], trueSecondData[i] * array->GetScale()))
    {
      vtkGenericWarningMacro("Incorrect values returned from scaled array");
      return 1;
    }
  }

  // second check that if we set information based on firstData and secondData
  // that we get that back
  for (vtkIdType i = 0; i < array->GetNumberOfTuples(); i++)
  {
    vals[0] = trueFirstData[i];
    vals[1] = trueSecondData[i];
    array->SetTypedTuple(i, vals);
    array->GetTypedTuple(i, vals);
    if (!vtkMathUtilities::NearlyEqual(vals[0], trueFirstData[i]) ||
      !vtkMathUtilities::NearlyEqual(vals[1], trueSecondData[i]))
    {
      vtkGenericWarningMacro(
        "Incorrect values returned from scaled array after setting values in the array");
      return 1;
    }
  }

  // third check is for FillValue()
  array->FillValue(2.);
  for (vtkIdType i = 0; i < array->GetNumberOfTuples(); i++)
  {
    array->GetTypedTuple(i, vals);
    if (!vtkMathUtilities::NearlyEqual(vals[0], 2.) || !vtkMathUtilities::NearlyEqual(vals[1], 2.))
    {
      vtkGenericWarningMacro(
        "Incorrect values returned from scaled array after setting with FillValue(2.)");
      return 1;
    }
  }

  // fourth check is for getting raw pointer
  // Silence the void pointer warnings for these calls
  vtksys::SystemTools::PutEnv("VTK_SILENCE_GET_VOID_POINTER_WARNINGS=1");
  double* rawPointer = array->GetPointer(0);
  if (!vtkMathUtilities::NearlyEqual(rawPointer[0], 2.))
  {
    vtkGenericWarningMacro("Incorrect values returned from scaled array after GetPointer()");
    return 1;
  }

  array->Delete();

  return 0; // success
}
