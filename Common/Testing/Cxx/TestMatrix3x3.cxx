#include "vtkMatrix3x3.h"
#include "vtkTransform2D.h"
#include "vtkPoints2D.h"
#include "vtkSmartPointer.h"

#include <math.h>
#include <vtkstd/limits>

// Perform a fuzzy compare of floats/doubles
template<class A>
bool fuzzyCompare(A a, A b) {
  return fabs(a - b) < vtkstd::numeric_limits<A>::epsilon();
}

// Perform a fuzzy compare of floats/doubles, specify the allowed tolerance
template<class A>
bool fuzzyCompare(A a, A b, A epsilon) {
  return fabs(a - b) < epsilon;
}

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int TestMatrix3x3(int,char *[])
{
  // Instantiate a vtkMatrix3x3 and test out the funtions.
  VTK_CREATE(vtkMatrix3x3, matrix);
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
  VTK_CREATE(vtkMatrix3x3, matrix2);
  matrix2->DeepCopy(matrix);
  if (*matrix != *matrix2)
    {
    vtkGenericWarningMacro("DeepCopy of vtkMatrix3x3 failed.");
    return 1;
    }
  if (!(*matrix == *matrix2))
    {
    vtkGenericWarningMacro("Problem with vtkMatrix3x3::operator==");
    return 1;
    }
  matrix2->SetElement(0, 0, 5.0);
  if (!(*matrix != *matrix2))
    {
    vtkGenericWarningMacro("Problem with vtkMatrix3x3::operator!=");
    return 1;
    }
  if (*matrix == *matrix2)
    {
    vtkGenericWarningMacro("Problem with vtkMatrix3x3::operator==");
    return 1;
    }

  if (!fuzzyCompare(matrix2->GetElement(0, 0), 5.0))
    {
    vtkGenericWarningMacro("Value not stored in matrix properly.");
    return 1;
    }
  matrix2->SetElement(1, 2, 42.0);
  if (!fuzzyCompare(matrix2->GetElement(1, 2), 42.0))
    {
    vtkGenericWarningMacro("Value not stored in matrix properly.");
    return 1;
    }

  // Test matrix transpose
  matrix2->Transpose();
  if (!fuzzyCompare(matrix2->GetElement(0, 0), 5.0) ||
      !fuzzyCompare(matrix2->GetElement(2, 1), 42.0))
    {
    vtkGenericWarningMacro("vtkMatrix::Transpose failed.");
    return 1;
    }

  matrix2->Invert();
  if (!fuzzyCompare(matrix2->GetElement(0, 0), 0.2) ||
      !fuzzyCompare(matrix2->GetElement(2, 1), -42.0))
    {
    vtkGenericWarningMacro("vtkMatrix::Transpose failed.");
    return 1;
    }

  // Not test the 2D transform with some 2D points
  VTK_CREATE(vtkTransform2D, transform);
  VTK_CREATE(vtkPoints2D, points);
  VTK_CREATE(vtkPoints2D, points2);
  points->SetNumberOfPoints(3);
  points->SetPoint(0, 0.0, 0.0);
  points->SetPoint(1, 3.0, 4.9);
  points->SetPoint(2, 42.0, 69.0);

  transform->TransformPoints(points, points2);
  for (int i = 0; i < 3; ++i)
    {
    double p1[2], p2[2];
    points->GetPoint(i, p1);
    points2->GetPoint(i, p2);
    if (!fuzzyCompare(p1[0], p2[0], 1e-5) ||
        !fuzzyCompare(p1[1], p2[1], 1e-5))
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
  transform->TransformPoints(points, points2);
  for (int i = 0; i < 3; ++i)
    {
    double p1[2], p2[2];
    points->GetPoint(i, p1);
    points2->GetPoint(i, p2);
    if (!fuzzyCompare(p1[0], p2[0] - 2.0, 1e-5) ||
        !fuzzyCompare(p1[1], p2[1] - 6.9, 1e-5))
      {
      vtkGenericWarningMacro("Translation transform failed. Delta: "
                             << p1[0] - (p2[0]-2.0)
                             << ", "
                             << p1[1] - (p2[1]-6.9));

      return 1;
      }
    }
  transform->InverseTransformPoints(points2, points2);
  for (int i = 0; i < 3; ++i)
    {
    double p1[2], p2[2];
    points->GetPoint(i, p1);
    points2->GetPoint(i, p2);
    if (!fuzzyCompare(p1[0], p2[0], 1e-5) ||
        !fuzzyCompare(p1[1], p2[1], 1e-5))
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
