// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "vtkMathUtilities.h"

#include <iostream>

int TestDataArray(int, char*[])
{
  double range[2];
  vtkIntArray* array = vtkIntArray::New();
  array->GetRange(range, 0);
  if (range[0] != VTK_DOUBLE_MAX || range[1] != VTK_DOUBLE_MIN)
  {
    std::cerr << "1) Getting range of empty array failed, min: " << range[0] << " max: " << range[1]
              << "\n";
    array->Delete();
    return 1;
  }
  array->GetFiniteRange(range, 0);
  if (range[0] != VTK_DOUBLE_MAX || range[1] != VTK_DOUBLE_MIN)
  {
    std::cerr << "2) Getting finite range of empty array failed, min: " << range[0]
              << " max: " << range[1] << "\n";
    array->Delete();
    return 1;
  }

  int cc;
  for (cc = 0; cc < 10; cc++)
  {
    array->InsertNextTuple1(cc);
  }
  array->GetRange(range, 0); // Range is now 0-9. Used to check MTimes.
  if (range[0] != 0 || range[1] != 9)
  {
    std::cerr << "3) Getting range (" << range[0] << "-" << range[1]
              << ") of array marked for modified didn't cause recomputation of range!";
    array->Delete();
    return 1;
  }
  array->GetFiniteRange(range, 0); // Range is now 0-9. Used to check MTimes.
  if (range[0] != 0 || range[1] != 9)
  {
    std::cerr << "4) Getting finite range (" << range[0] << "-" << range[1]
              << ") of array marked for modified didn't cause recomputation of range!";
    array->Delete();
    return 1;
  }

  std::cerr << "Getting range (" << range[0] << "-" << range[1] << ")" << std::endl;
  array->RemoveFirstTuple();
  array->RemoveTuple(3);
  array->RemoveTuple(4);
  array->GetRange(range, 0);
  if (range[0] != 0 || range[1] != 9)
  {
    std::cerr << "5) Getting range (" << range[0] << "-" << range[1]
              << ") of array not marked as modified caused recomputation of range!";
    array->Delete();
    return 1;
  }
  array->GetFiniteRange(range, 0);
  if (range[0] != 0 || range[1] != 9)
  {
    std::cerr << "6) Getting finite range (" << range[0] << "-" << range[1]
              << ") of array not marked as modified caused recomputation of range!";
    array->Delete();
    return 1;
  }
  array->Modified(); // Now mark array so range gets recomputed
  array->GetRange(range, 0);
  if (range[0] != 1. || range[1] != 9.)
  {
    std::cerr << "7) Getting range of array {1,2,3,5,7,8,9} failed, min: " << range[0]
              << " max: " << range[1] << "\n";
    array->Delete();
    return 1;
  }
  array->GetFiniteRange(range, 0);
  if (range[0] != 1. || range[1] != 9.)
  {
    std::cerr << "9) Getting finite range of array {1,2,3,5,7,8,9} failed, min: " << range[0]
              << " max: " << range[1] << "\n";
    array->Delete();
    return 1;
  }

  array->RemoveLastTuple();
  array->Modified();
  array->GetRange(range, 0);
  if (range[0] != 1. || range[1] != 8.)
  {
    std::cerr << "10) Getting range of array {1,2,3,5,7,8} failed, min: " << range[0]
              << " max: " << range[1] << "\n";
    array->Delete();
    return 1;
  }
  array->GetFiniteRange(range, 0);
  if (range[0] != 1. || range[1] != 8.)
  {
    std::cerr << "11) Getting finite range of array {1,2,3,5,7,8} failed, min: " << range[0]
              << " max: " << range[1] << "\n";
    array->Delete();
    return 1;
  }
  int ca[] = { 1, 2, 3, 5, 7, 8 };
  std::cout << "Array:";
  for (cc = 0; cc < array->GetNumberOfTuples(); ++cc)
  {
    if (array->GetTuple1(cc) != ca[cc])
    {
      std::cerr << "12) Problem with array: " << array->GetTuple1(cc) << " <> " << ca[cc]
                << std::endl;
      array->Delete();
      return 1;
    }
    std::cout << " " << array->GetTuple1(cc);
  }
  std::cout << std::endl;
  array->Delete();

  // Ensure GetFiniteRange ignores Inf and Nan.
  vtkDoubleArray* farray = vtkDoubleArray::New();
  for (cc = 0; cc < 10; cc++)
  {
    farray->InsertNextTuple1(cc);
  }
  farray->InsertNextTuple1(vtkMath::Inf());
  farray->InsertNextTuple1(vtkMath::NegInf());
  farray->InsertNextTuple1(vtkMath::Nan());
  farray->GetRange(range, 0);
  if (range[0] != vtkMath::NegInf() || range[1] != vtkMath::Inf())
  {
    std::cerr << "13) Getting range (" << range[0] << "-" << range[1]
              << ") of array containing infinity and NaN" << std::endl;
    farray->Delete();
    return 1;
  }
  farray->GetFiniteRange(range, 0); // Range is now 0-9. Used to check MTimes.
  if (!vtkMathUtilities::FuzzyCompare(range[0], 0.0) ||
    !vtkMathUtilities::FuzzyCompare(range[1], 9.0))
  {
    std::cerr << "14) Getting finite range (" << range[0] << "-" << range[1]
              << ") of array containing infinity and NaN" << std::endl;
    farray->Delete();
    return 1;
  }
  farray->Delete();

  farray = vtkDoubleArray::New();
  farray->SetNumberOfComponents(3);
  for (cc = 0; cc < 10; cc++)
  {
    farray->InsertNextTuple3(cc + 0.125, cc + 0.250, cc + 0.375);
  }
  farray->RemoveFirstTuple();
  farray->RemoveTuple(3);
  farray->RemoveTuple(4);
  farray->RemoveLastTuple();
  std::cout << "Array:";
  for (cc = 0; cc < farray->GetNumberOfTuples(); ++cc)
  {
    double* fa = farray->GetTuple3(cc);
    double fc[3];
    fc[0] = ca[cc] + .125;
    fc[1] = ca[cc] + .250;
    fc[2] = ca[cc] + .375;
    for (int i = 0; i < 3; i++)
    {
      if (fa[i] != fc[i])
      {
        std::cerr << "15) Problem with array: " << fa[i] << " <> " << fc[i] << std::endl;
        farray->Delete();
        return 1;
      }
    }
    std::cout << " " << fa[0] << "," << fa[1] << "," << fa[2];
  }
  std::cout << std::endl;
  farray->Delete();
  return 0;
}
