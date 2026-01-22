// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDataArrayRange.h"
#include "vtkMathUtilities.h"
#include "vtkSOADataArrayTemplate.h"

// Needed for portable setenv on MSVC...
#include "vtksys/SystemTools.hxx"

#include <cmath>
#include <limits>
#include <math.h>

#include <iostream>

namespace
{
constexpr int numTuples = 5;
double firstData[numTuples] = { 0, 1.5, 2, std::numeric_limits<double>::max(), nan("") };
double secondData[numTuples] = { 10, 11.5, 12, std::numeric_limits<double>::infinity(), 15 };
const double voidPointerData[2 * numTuples] = { -1, -2.5, std::numeric_limits<double>::infinity(),
  -4, -5, -6, std::numeric_limits<double>::min(), -8, nan(""), -10 };
const double otherFirst[numTuples] = { voidPointerData[0], voidPointerData[2], voidPointerData[4],
  voidPointerData[6], voidPointerData[8] };
const double otherSecond[numTuples] = { voidPointerData[1], voidPointerData[3], voidPointerData[5],
  voidPointerData[7], voidPointerData[9] };

bool ValuesInfOrNaN(double v1, double v2)
{
  return (std::isinf(v1) && std::isinf(v2)) || (std::isnan(v1) && std::isnan(v2));
}

bool HasCorrectValues(vtkSOADataArrayTemplate<double>* array, bool useVoidPointerData)
{
  int numberOfComponents = array->GetNumberOfComponents();
  const double* first = firstData;
  const double* second = secondData;
  if (useVoidPointerData)
  {
    first = numberOfComponents == 1 ? voidPointerData : otherFirst;
    second = otherSecond;
  }
  auto values = vtk::DataArrayValueRange(array);
  double tupleValues[2];
  for (int i = 0; i < numTuples; i++)
  {
    array->GetTuple(i, tupleValues);
    if ((!vtkMathUtilities::NearlyEqual(tupleValues[0], first[i]) &&
          !ValuesInfOrNaN(tupleValues[0], first[i])) ||
      (numberOfComponents == 2 &&
        (!vtkMathUtilities::NearlyEqual(tupleValues[1], second[i]) &&
          !ValuesInfOrNaN(tupleValues[1], second[i]))))
    {
      std::cerr << "Incorrect values returned from GetTypedTuple()\n";
      return false;
    }
    if ((!vtkMathUtilities::NearlyEqual(
           static_cast<double>(values[i * numberOfComponents]), first[i]) &&
          !ValuesInfOrNaN(values[i * numberOfComponents], first[i])) ||
      (numberOfComponents == 2 &&
        (!vtkMathUtilities::NearlyEqual(
           static_cast<double>(values[i * numberOfComponents + 1]), second[i]) &&
          !ValuesInfOrNaN(values[i * numberOfComponents + 1], second[i]))))
    {
      std::cerr << "Incorrect values returned from DataArrayValueRange()\n";
      return false;
    }
  }

  return true;
}
}

int TestSOADataArray(int, char*[])
{
  int retVal = 0;
  vtkSOADataArrayTemplate<double>* array = vtkSOADataArrayTemplate<double>::New();
  array->SetNumberOfComponents(2);
  array->SetNumberOfTuples(numTuples);
  array->SetArray(0, firstData, numTuples, false, true);
  array->SetArray(1, secondData, numTuples, false, true);
  if (!HasCorrectValues(array, false))
  {
    std::cerr << "Setting values through SetArray() failed\n";
    retVal++;
  }

  // this should switch to AOS storage
  auto ptr = vtk::DataArrayValueRange(array).begin();
  std::copy_n(voidPointerData, 2 * numTuples, ptr);
  if (!HasCorrectValues(array, true))
  {
    std::cerr << "Setting values through DataArrayValueRange() failed\n";
    retVal++;
  }
  array->SetNumberOfComponents(2);
  array->SetArray(0, firstData, numTuples, false, true);
  array->SetArray(1, secondData, numTuples, false, true);
  if (!HasCorrectValues(array, false))
  {
    std::cerr << "Setting values through SetArray() failed\n";
    retVal++;
  }

  // now create a new instance from the old (like we would do in a filter)
  auto* newInstance = array->NewInstance();
  newInstance->SetNumberOfComponents(2);
  newInstance->SetNumberOfTuples(numTuples);
  for (vtkIdType i = 0; i < numTuples; i++)
  {
    double tupleValues[2] = { firstData[i], secondData[i] };
    newInstance->SetTuple(i, tupleValues);
  }
  if (!HasCorrectValues(newInstance, false))
  {
    std::cerr << "Setting values through SetTuple() failed\n";
    retVal++;
  }
  newInstance->Fill(0);
  ptr = vtk::DataArrayValueRange(newInstance).begin();
  std::copy_n(voidPointerData, 2 * numTuples, ptr);
  if (!HasCorrectValues(newInstance, true))
  {
    std::cerr << "Setting values through DataArrayValueRange() failed\n";
    retVal++;
  }

  // now test the use case of a single component. this is the situation
  // where we don't switch between AOS and SOA storage since we don't need to
  array->SetNumberOfComponents(1);
  array->SetNumberOfTuples(numTuples);
  array->SetArray(0, firstData, numTuples, false, true);
  if (!HasCorrectValues(array, false))
  {
    std::cerr << "Setting single component values through SetArray() failed\n";
    retVal++;
  }

  ptr = vtk::DataArrayValueRange(array).begin();
  std::copy_n(voidPointerData, numTuples,
    ptr); // note that this changes values in firstData
  if (!HasCorrectValues(array, true))
  {
    std::cerr << "Setting single component values through DataArrayValueRange() failed\n";
    retVal++;
  }
  array->SetNumberOfComponents(1);
  array->SetArray(0, firstData, numTuples, false, true);
  if (!HasCorrectValues(array, false))
  {
    std::cerr << "Setting single component values through SetArray() failed\n";
    retVal++;
  }

  newInstance->Delete();
  array->Delete();

  return retVal; // success has retVal = 0
}
