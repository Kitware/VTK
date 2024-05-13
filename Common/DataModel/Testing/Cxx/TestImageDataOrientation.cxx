// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// .NAME Test direction API for image data
// .SECTION Description
// This program tests the direction API of the image data.

#include "vtkCell.h"
#include "vtkDebugLeaks.h"
#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkMathUtilities.h"
#include "vtkMatrix3x3.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkPoints.h"

inline int DoOrientationTest(
  int extent[6], double origin[3], double spacing[3], double direction[9])
{
  double tol = 10e-15;

  // Create image
  vtkNew<vtkImageData> image;
  image->SetExtent(extent);
  image->SetOrigin(origin);
  image->SetSpacing(spacing);
  image->SetDirectionMatrix(direction);
  image->AllocateScalars(VTK_DOUBLE, 1);

  // Check some values in index to physical matrix
  vtkMatrix4x4* m4 = image->GetIndexToPhysicalMatrix();
  if (m4->GetElement(0, 3) != origin[0] || m4->GetElement(1, 3) != origin[1] ||
    m4->GetElement(2, 3) != origin[2] || m4->GetElement(3, 3) != 1)
  {

    vtkGenericWarningMacro(
      "IndexToPhysical matrix of the image data is missing the translation information");
    return EXIT_FAILURE;
  }

  // Go from min IJK to XYZ coordinates
  int i, j, k;
  i = extent[0];
  j = extent[2];
  k = extent[4];
  double xyz[3];
  image->TransformIndexToPhysicalPoint(i, j, k, xyz);

  // Test FindCell and ensure it finds the first cell (since we used IJK min)
  double pcoords[3];
  int subId = 0;
  vtkIdType cellId = image->FindCell(xyz, nullptr, 0, 0, subId, pcoords, nullptr);
  if (cellId != 0)
  {
    vtkGenericWarningMacro("FindCell returns " << cellId << ", expected 0");
    return EXIT_FAILURE;
  }
  if (!vtkMathUtilities::FuzzyCompare(pcoords[0], 0.0, tol) ||
    !vtkMathUtilities::FuzzyCompare(pcoords[1], 0.0, tol) ||
    !vtkMathUtilities::FuzzyCompare(pcoords[2], 0.0, tol))
  {
    vtkGenericWarningMacro(
      "FindCell returns the proper cell (0), but pcoords isn't equal to {0,0,0}");
    return EXIT_FAILURE;
  }

  // Test GetCell and ensure it returns the same value as XYZ above
  vtkCell* cell = image->GetCell(cellId);
  double pt[3];
  cell->GetPoints()->GetPoint(0, pt);
  if (!vtkMathUtilities::FuzzyCompare(pt[0], xyz[0], tol) ||
    !vtkMathUtilities::FuzzyCompare(pt[1], xyz[1], tol) ||
    !vtkMathUtilities::FuzzyCompare(pt[2], xyz[2], tol))
  {
    vtkGenericWarningMacro(
      "GetCell result for cell " << cellId << " does not match expected values.");
    return EXIT_FAILURE;
  }
  // Go from physical coordinate to index coordinate and ensure
  // it matches with ijk
  double index[3];
  image->TransformPhysicalPointToContinuousIndex(pt, index);
  if (!vtkMathUtilities::FuzzyCompare(index[0], (double)i, tol) ||
    !vtkMathUtilities::FuzzyCompare(index[1], (double)j, tol) ||
    !vtkMathUtilities::FuzzyCompare(index[2], (double)k, tol))
  {
    vtkGenericWarningMacro("Applying the PhysicalToIndex matrix does not return expected indices.");
    return EXIT_FAILURE;
  }

  // Store index to physical matrix
  vtkNew<vtkMatrix4x4> indexToPhysical;
  indexToPhysical->DeepCopy(image->GetIndexToPhysicalMatrix());

  // Check with setting with 4x4 matrix
  for (int testedMatrixIndex = 0; testedMatrixIndex < 2; testedMatrixIndex++)
  {
    // Check setting to identity matrix

    vtkNew<vtkMatrix4x4> identity;
    std::string testedMatrixName;
    if (testedMatrixIndex == 0)
    {
      testedMatrixName = "IndexToPhysical";
      image->ApplyIndexToPhysicalMatrix(identity);
    }
    else
    {
      testedMatrixName = "PhysicalToIndex";
      image->ApplyPhysicalToIndexMatrix(identity);
    }

    vtkMatrix3x3* directionMatrix = image->GetDirectionMatrix();
    for (int axis = 0; axis < 3; ++axis)
    {
      if (image->GetOrigin()[axis] != 0.0 || image->GetSpacing()[axis] != 1.0 ||
        directionMatrix->GetElement(0, axis) != (axis == 0 ? 1.0 : 0.0) ||
        directionMatrix->GetElement(1, axis) != (axis == 1 ? 1.0 : 0.0) ||
        directionMatrix->GetElement(2, axis) != (axis == 2 ? 1.0 : 0.0))
      {
        vtkGenericWarningMacro(
          "Applying identity " << testedMatrixName << " matrix does not set expected geometry.");
        return EXIT_FAILURE;
      }
    }

    // Check setting image geometry using 4x4 matrix

    std::string testedMatrix;
    if (testedMatrixIndex == 0)
    {
      image->ApplyIndexToPhysicalMatrix(indexToPhysical);
    }
    else
    {
      vtkNew<vtkMatrix4x4> physicalToIndex;
      vtkMatrix4x4::Invert(indexToPhysical, physicalToIndex);
      image->ApplyPhysicalToIndexMatrix(physicalToIndex);
    }

    directionMatrix = image->GetDirectionMatrix();
    vtkNew<vtkMatrix3x3> expectedDirectionMatrix;
    expectedDirectionMatrix->DeepCopy(direction);
    bool failed = false;
    for (int axis = 0; axis < 3; ++axis)
    {
      if (!vtkMathUtilities::FuzzyCompare(image->GetOrigin()[axis], origin[axis], tol))
      {
        vtkGenericWarningMacro(
          "Applying the " << testedMatrixName << " matrix does not set expected origin.");
        failed = true;
      }
      // When spacing is set from 4x4 matrix, the spacing is always positive.
      // However, the input spacing can be negative, apply that sign when comparing.
      double inputSpacingSign = spacing[axis] < 0 ? -1 : 1;
      if (!vtkMathUtilities::FuzzyCompare(
            image->GetSpacing()[axis] * inputSpacingSign, spacing[axis], tol))
      {
        vtkGenericWarningMacro(
          "Applying the " << testedMatrixName << " matrix does not set expected spacing.");
        failed = true;
      }
      for (int row = 0; row < 3; ++row)
      {
        if (!vtkMathUtilities::FuzzyCompare(
              directionMatrix->GetElement(row, axis) * inputSpacingSign,
              expectedDirectionMatrix->GetElement(row, axis), tol))
        {
          vtkGenericWarningMacro(
            "Applying the " << testedMatrixName << " matrix does not set expected direction.");
          failed = true;
        }
      }
    }
    if (failed)
    {
      std::cerr << "Expected IndexToPhysical matrix: " << std::endl;
      indexToPhysical->PrintSelf(std::cerr, vtkIndent());
      std::cerr << "Actual IndexToPhysical matrix: " << std::endl;
      image->GetIndexToPhysicalMatrix()->PrintSelf(std::cerr, vtkIndent());
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}

int TestImageDataOrientation(int, char*[])
{
  const double pi = vtkMath::Pi();

  // test 0D, 1D, 2D, 3D data with various extents, spacings, origins, directions
  static int dims[4][3] = {
    { 1, 1, 1 },
    { 3, 1, 1 },
    { 3, 3, 1 },
    { 3, 3, 3 },
  };
  static int starts[4][3] = {
    { 0, 0, 0 },
    { -1, 0, -1 },
    { 2, 3, 6 },
    { -10, 0, 5 },
  };
  static double spacings[4][3] = {
    { 1, 1, 1 },
    { 1.0 / 7, 1, 1 },
    { 1, -1, 1 },
    { -1, 1, -1 / 13.0 },
  };
  static double origins[4][3] = {
    { 0, 0, 0 },
    { 1.0 / 13, 0, 0 },
    { 0, -1, 0 },
    { -1, 0, -1 / 7.0 },
  };
  static double directions[7][9] = {
    {
      1, 0, 0, //
      0, 1, 0, //
      0, 0, 1  //
    },
    {
      -1, 0, 0, //
      0, -1, 0, //
      0, 0, 1   //
    },
    {
      1, 0, 0, //
      0, 0, 1, //
      0, 1, 0  //
    },
    {
      0, -1, 0, //
      1, 0, 0,  //
      0, 0, 1   //
    },
    {
      1, 0, 0,                     //
      0, cos(pi / 4), sin(pi / 4), //
      0, -sin(pi / 4), cos(pi / 4) //
    },
    {
      cos(-pi / 5), sin(-pi / 5), 0,  //
      -sin(-pi / 5), cos(-pi / 5), 0, //
      0, 0, 1                         //
    },
    {
      cos(pi / 0.8), 0, sin(pi / 0.8),  //
      0, 1, 0,                          //
      -sin(pi / 0.8), 0, cos(pi / 0.8), //
    },
  };

  int extent[6];
  double* spacing;
  double* origin;
  double* direction;

  int failed = 0;

  for (int i = 0; i < 4; i++)
  {
    for (int j = 0; j < 4; j++)
    {
      for (int k = 0; k < 4; k++)
      {
        spacing = spacings[k];
        for (int l = 0; l < 4; l++)
        {
          origin = origins[l];
          for (int ii = 0; ii < 3; ii++)
          {
            extent[2 * ii] = starts[i][ii];
            extent[2 * ii + 1] = starts[i][ii] + dims[j][ii] - 1;
          }

          for (int jj = 0; jj < 4; jj++)
          {
            direction = directions[jj];
            if (DoOrientationTest(extent, origin, spacing, direction) == EXIT_FAILURE)
            {
              failed = EXIT_FAILURE;
            }
          }
        }
      }
    }
  }

  return failed;
}
