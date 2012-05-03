/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMatrix3x3.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMatrix3x3.h"
#include "vtkTransform2D.h"
#include "vtkPoints2D.h"
#include "vtkNew.h"
#include "vtkMathUtilities.h"

int TestMatrix3x3(int,char *[])
{
  // Instantiate a vtkMatrix3x3 and test out the funtions.
  vtkNew<vtkMatrix3x3> matrix;
  cout << "Testing vtkMatrix3x3..." << endl;
  if (!matrix->IsIdentity())
    {
    vtkGenericWarningMacro("Matrix should be initialized to identity.");
    return 1;
    }
  matrix->Invert();
  if (!matrix->IsIdentity())
    {
    vtkGenericWarningMacro("Inverse of identity should be identity.");
    return 1;
    }
  // Check copying and comparison
  vtkNew<vtkMatrix3x3> matrix2;
  matrix2->DeepCopy(matrix.GetPointer());
  if (*matrix.GetPointer() != *matrix2.GetPointer())
    {
    vtkGenericWarningMacro("DeepCopy of vtkMatrix3x3 failed.");
    return 1;
    }
  if (!(*matrix.GetPointer() == *matrix2.GetPointer()))
    {
    vtkGenericWarningMacro("Problem with vtkMatrix3x3::operator==");
    return 1;
    }
  matrix2->SetElement(0, 0, 5.0);
  if (!(*matrix.GetPointer() != *matrix2.GetPointer()))
    {
    vtkGenericWarningMacro("Problem with vtkMatrix3x3::operator!=");
    return 1;
    }
  if (*matrix.GetPointer() == *matrix2.GetPointer())
    {
    vtkGenericWarningMacro("Problem with vtkMatrix3x3::operator==");
    return 1;
    }

  if (!vtkMathUtilities::FuzzyCompare(matrix2->GetElement(0, 0), 5.0))
    {
    vtkGenericWarningMacro("Value not stored in matrix properly.");
    return 1;
    }
  matrix2->SetElement(1, 2, 42.0);
  if (!vtkMathUtilities::FuzzyCompare(matrix2->GetElement(1, 2), 42.0))
    {
    vtkGenericWarningMacro("Value not stored in matrix properly.");
    return 1;
    }

  // Test matrix transpose
  matrix2->Transpose();
  if (!vtkMathUtilities::FuzzyCompare(matrix2->GetElement(0, 0), 5.0) ||
      !vtkMathUtilities::FuzzyCompare(matrix2->GetElement(2, 1), 42.0))
    {
    vtkGenericWarningMacro("vtkMatrix::Transpose failed.");
    return 1;
    }

  matrix2->Invert();
  if (!vtkMathUtilities::FuzzyCompare(matrix2->GetElement(0, 0), 0.2) ||
      !vtkMathUtilities::FuzzyCompare(matrix2->GetElement(2, 1), -42.0))
    {
    vtkGenericWarningMacro("vtkMatrix::Transpose failed.");
    return 1;
    }

  // Not test the 2D transform with some 2D points
  vtkNew<vtkTransform2D> transform;
  vtkNew<vtkPoints2D> points;
  vtkNew<vtkPoints2D> points2;
  points->SetNumberOfPoints(3);
  points->SetPoint(0, 0.0, 0.0);
  points->SetPoint(1, 3.0, 4.9);
  points->SetPoint(2, 42.0, 69.0);

  transform->TransformPoints(points.GetPointer(), points2.GetPointer());
  for (int i = 0; i < 3; ++i)
    {
    double p1[2], p2[2];
    points->GetPoint(i, p1);
    points2->GetPoint(i, p2);
    if (!vtkMathUtilities::FuzzyCompare(p1[0], p2[0], 1e-5) ||
        !vtkMathUtilities::FuzzyCompare(p1[1], p2[1], 1e-5))
      {
      vtkGenericWarningMacro("Identity transform moved points."
                             << " Delta: "
                             << p1[0] - (p2[0]-2.0)
                             << ", "
                             << p1[1] - (p2[1]-6.9));
      return 1;
      }
    }
  transform->Translate(2.0, 6.9);
  transform->TransformPoints(points.GetPointer(), points2.GetPointer());
  for (int i = 0; i < 3; ++i)
    {
    double p1[2], p2[2];
    points->GetPoint(i, p1);
    points2->GetPoint(i, p2);
    if (!vtkMathUtilities::FuzzyCompare(p1[0], p2[0] - 2.0, 1e-5) ||
        !vtkMathUtilities::FuzzyCompare(p1[1], p2[1] - 6.9, 1e-5))
      {
      vtkGenericWarningMacro("Translation transform failed. Delta: "
                             << p1[0] - (p2[0]-2.0)
                             << ", "
                             << p1[1] - (p2[1]-6.9));

      return 1;
      }
    }
  transform->InverseTransformPoints(points2.GetPointer(), points2.GetPointer());
  for (int i = 0; i < 3; ++i)
    {
    double p1[2], p2[2];
    points->GetPoint(i, p1);
    points2->GetPoint(i, p2);
    if (!vtkMathUtilities::FuzzyCompare(p1[0], p2[0], 1e-5) ||
        !vtkMathUtilities::FuzzyCompare(p1[1], p2[1], 1e-5))
      {
      vtkGenericWarningMacro("Inverse transform did not return original points."
                             << " Delta: "
                             << p1[0] - (p2[0]-2.0)
                             << ", "
                             << p1[1] - (p2[1]-6.9));
      return 1;
      }
    }

  return 0;
}
