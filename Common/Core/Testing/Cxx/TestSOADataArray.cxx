/*==============================================================================

  Program:   Visualization Toolkit
  Module:    TestSOADataArray.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

==============================================================================*/
#include "vtkArrayIterator.h"
#include "vtkArrayIteratorIncludes.h"
#include "vtkMathUtilities.h"
#include "vtkSOADataArrayTemplate.h"

// Needed for portable setenv on MSVC...
#include "vtksys/SystemTools.hxx"

#include <cmath>
#include <limits>
#include <math.h>

namespace
{
const int numValues = 5;
double firstData[numValues] = { 0, 1.5, 2, std::numeric_limits<double>::max(), nan("") };
double secondData[numValues] = { 10, 11.5, 12, std::numeric_limits<double>::infinity(), 15 };
const double voidPointerData[2 * numValues] = { -1, -2.5, std::numeric_limits<double>::infinity(),
  -4, -5, -6, std::numeric_limits<double>::min(), -8, nan(""), -10 };
const double otherFirst[numValues] = { voidPointerData[0], voidPointerData[2], voidPointerData[4],
  voidPointerData[6], voidPointerData[8] };
const double otherSecond[numValues] = { voidPointerData[1], voidPointerData[3], voidPointerData[5],
  voidPointerData[7], voidPointerData[9] };

bool ValuesInfOrNaN(double v1, double v2)
{
  return (isinf(v1) && isinf(v2)) || (isnan(v1) && isnan(v2));
}

bool HasCorrectValues(vtkDataArray* array, bool useVoidPointerData)
{
  int numberOfComponents = array->GetNumberOfComponents();
  const double* first = firstData;
  const double* second = secondData;
  if (useVoidPointerData)
  {
    first = numberOfComponents == 1 ? voidPointerData : otherFirst;
    second = otherSecond;
  }
  double* values = static_cast<double*>(array->GetVoidPointer(0));
  double tupleValues[2];
  for (int i = 0; i < numValues; i++)
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
    if ((!vtkMathUtilities::NearlyEqual(values[i * numberOfComponents], first[i]) &&
          !ValuesInfOrNaN(values[i * numberOfComponents], first[i])) ||
      (numberOfComponents == 2 &&
        (!vtkMathUtilities::NearlyEqual(values[i * numberOfComponents + 1], second[i]) &&
          !ValuesInfOrNaN(values[i * numberOfComponents + 1], second[i]))))
    {
      std::cerr << "Incorrect values returned from GetVoidPointer()\n";
      return false;
    }
  }

  vtkArrayIteratorTemplate<double>* iter = vtkArrayIteratorTemplate<double>::New();
  iter->Initialize(array);
  for (int i = 0; i < numValues; i++)
  {
    values = iter->GetTuple(i);
    if ((!vtkMathUtilities::NearlyEqual(values[0], first[i]) &&
          !ValuesInfOrNaN(values[0], first[i])) ||
      (numberOfComponents == 2 &&
        (!vtkMathUtilities::NearlyEqual(values[1], second[i]) &&
          !ValuesInfOrNaN(values[1], second[i]))))
    {
      std::cerr << "Incorrect values returned from NewIterator()\n";
      return false;
    }
  }
  iter->Delete();

  return true;
}
}

int TestSOADataArray(int, char*[])
{
  int retVal = 0;
  vtkSOADataArrayTemplate<double>* array = vtkSOADataArrayTemplate<double>::New();
  array->SetNumberOfComponents(2);
  array->SetNumberOfTuples(numValues);
  array->SetArray(0, firstData, numValues, false, true);
  array->SetArray(1, secondData, numValues, false, true);
  if (!HasCorrectValues(array, false))
  {
    std::cerr << "Setting values through SetArray() failed\n";
    retVal++;
  }

  // now test setting values through vtkArrayIterator
  array->Fill(0);
  vtkArrayIteratorTemplate<double>* iter = vtkArrayIteratorTemplate<double>::New();
  iter->Initialize(array);
  for (int i = 0; i < numValues; i++)
  {
    for (int j = 0; j < 2; j++)
    {
      double value = (j == 0 ? firstData[i] : secondData[i]);
      iter->SetValue(i * 2 + j, value);
    }
  }
  iter->Delete();
  if (!HasCorrectValues(array, false))
  {
    std::cerr << "Setting values through vtkArrayIterator() failed\n";
    retVal++;
  }

  // this should switch to AOS storage
  double* ptr = static_cast<double*>(array->GetVoidPointer(0));
  std::copy(voidPointerData, voidPointerData + 2 * numValues, ptr);
  if (!HasCorrectValues(array, true))
  {
    std::cerr << "Setting values through GetVoidPointer() failed\n";
    retVal++;
  }
  // now go back to SOA format
  array->SetNumberOfComponents(2);
  array->SetArray(0, firstData, numValues, false, true);
  array->SetArray(1, secondData, numValues, false, true);
  if (!HasCorrectValues(array, false))
  {
    std::cerr << "Setting values through SetArray() failed\n";
    retVal++;
  }
  // now test setting values through vtkArrayIterator
  array->Fill(0);
  iter = vtkArrayIteratorTemplate<double>::New();
  iter->Initialize(array);
  for (int i = 0; i < 2 * numValues; i++)
  {
    iter->SetValue(i, voidPointerData[i]);
  }
  iter->Delete();
  if (!HasCorrectValues(array, true))
  {
    std::cerr << "Setting values through vtkArrayIterator() failed\n";
    retVal++;
  }

  // now create a new instance from the old (like we would do in a filter)
  vtkDataArray* newInstance = array->NewInstance();
  newInstance->SetNumberOfComponents(2);
  newInstance->SetNumberOfTuples(numValues);
  for (vtkIdType i = 0; i < numValues; i++)
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
  ptr = static_cast<double*>(newInstance->GetVoidPointer(0));
  std::copy(voidPointerData, voidPointerData + 2 * numValues, ptr);
  if (!HasCorrectValues(newInstance, true))
  {
    std::cerr << "Setting values through GetVoidPointer() failed\n";
    retVal++;
  }

  // now test the use case of a single component. this is the situation
  // where we don't switch between AOS and SOA storage since we don't need to
  array->SetNumberOfComponents(1);
  array->SetNumberOfTuples(numValues);
  array->SetArray(0, firstData, numValues, false, true);
  if (!HasCorrectValues(array, false))
  {
    std::cerr << "Setting single component values through SetArray() failed\n";
    retVal++;
  }

  // now test setting values through vtkArrayIterator
  array->Fill(0);
  iter = vtkArrayIteratorTemplate<double>::New();
  iter->Initialize(array);
  for (int i = 0; i < numValues; i++)
  {
    iter->SetValue(i, firstData[i]);
  }
  iter->Delete();
  if (!HasCorrectValues(array, false))
  {
    std::cerr << "Setting single component values through vtkArrayIterator() failed\n";
    retVal++;
  }

  ptr = static_cast<double*>(array->GetVoidPointer(0));
  std::copy(voidPointerData, voidPointerData + numValues,
    ptr); // note that this changes values in firstData
  if (!HasCorrectValues(array, true))
  {
    std::cerr << "Setting single component values through GetVoidPointer() failed\n";
    retVal++;
  }
  // now go back to SOA format
  array->SetNumberOfComponents(1);
  array->SetArray(0, firstData, numValues, false, true);
  if (!HasCorrectValues(array, false))
  {
    std::cerr << "Setting single component values through SetArray() failed\n";
    retVal++;
  }
  // now test setting values through vtkArrayIterator
  array->Fill(0);
  iter = vtkArrayIteratorTemplate<double>::New();
  iter->Initialize(array);
  for (int i = 0; i < numValues; i++)
  {
    iter->SetValue(i, voidPointerData[i]);
  }
  iter->Delete();
  if (!HasCorrectValues(array, true))
  {
    std::cerr << "Setting single component values through vtkArrayIterator() failed\n";
    retVal++;
  }

  newInstance->Delete();
  array->Delete();

  return retVal; // success has retVal = 0
}
