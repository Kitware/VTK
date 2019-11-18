/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImageIterator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME Test FindCell methods for image data
// .SECTION Description
// This program tests the FindCell methods for vtkImageData to
// ensure that they give correct results near the boundaries and
// to ensure that tolerance is handled properly.  Even when the
// tolerance is zero, points on the boundary must be considered
// to be inside the dataset.

#include "vtkDebugLeaks.h"
#include "vtkImageData.h"
#include "vtkMatrix4x4.h"
#include "vtkSmartPointer.h"

inline int DoTest(int extent[6], double origin[3], double spacing[3], double direction[9])
{
  vtkSmartPointer<vtkImageData> image = vtkSmartPointer<vtkImageData>::New();
  image->SetExtent(extent);
  image->SetOrigin(origin);
  image->SetSpacing(spacing);
  image->SetDirectionMatrix(direction);
  image->AllocateScalars(VTK_DOUBLE, 1);

  double bounds[6];
  image->GetBounds(bounds);

  int subId = 0;
  double pcoords[3];
  double weights[8];
  double ijk[3], x[3];
  vtkIdType cellId;

  double tol = 1e-4;

  for (int i = 0; i < 3; i++)
  {
    ijk[0] = 0.5 * (extent[0] + extent[1]);
    ijk[1] = 0.5 * (extent[2] + extent[3]);
    ijk[2] = 0.5 * (extent[4] + extent[5]);
    for (int j = 0; j < 2; j++)
    {
      // test point right on the boundary with zero tolerance
      ijk[i] = extent[2 * i + j];

      image->TransformContinuousIndexToPhysicalPoint(ijk, x);
      cellId = image->FindCell(x, nullptr, 0, 0.0, subId, pcoords, weights);

      if (cellId < 0)
      {
        cerr << "point (" << x[0] << ", " << x[1] << ", " << x[2] << ")"
             << " should be in bounds (" << bounds[0] << ", " << bounds[1] << ", " << bounds[2]
             << ", " << bounds[3] << ", " << bounds[4] << ", " << bounds[5] << ") with tol 0.0\n";
        return 1;
      }

      // test point just outside boundary with zero tolerance
      double offset = ((j == 0) ? (-tol * 0.5) : (tol * 0.5));
      ijk[i] = extent[2 * i + j] + offset;

      image->TransformContinuousIndexToPhysicalPoint(ijk, x);
      cellId = image->FindCell(x, nullptr, 0, 0.0, subId, pcoords, weights);

      if (cellId >= 0)
      {
        cerr << "point (" << x[0] << ", " << x[1] << ", " << x[2] << ")"
             << " should be out of bounds (" << bounds[0] << ", " << bounds[1] << ", " << bounds[2]
             << ", " << bounds[3] << ", " << bounds[4] << ", " << bounds[5] << ") with tol 0.0\n";
        return 1;
      }

      // test point just outside boundary with nonzero tolerance
      ijk[i] = extent[2 * i + j] + ((j == 0) ? (-tol * 0.5) : (tol * 0.5));

      image->TransformContinuousIndexToPhysicalPoint(ijk, x);
      cellId = image->FindCell(x, nullptr, 0, tol * tol, subId, pcoords, weights);

      if (cellId < 0)
      {
        cerr << "point (" << x[0] << ", " << x[1] << ", " << x[2] << ")"
             << " should be inside bounds (" << bounds[0] << ", " << bounds[1] << ", " << bounds[2]
             << ", " << bounds[3] << ", " << bounds[4] << ", " << bounds[5] << ") with tol " << tol
             << "\n";
        return 1;
      }

      // check pcoords at boundaries
      int isUpperBound = (j == 1);
      int isOnePixelThick = (extent[2 * i] == extent[2 * i + 1]);
      if (isUpperBound && !isOnePixelThick)
      {
        if (pcoords[i] != 1.0)
        {
          cerr << "at upper bounds, pcoord should be 1, but is " << pcoords[i] << "\n";
          return 1;
        }
      }
      else
      {
        if (pcoords[i] != 0.0)
        {
          cerr << "at lower bounds and for 0,1,2D cells, pcoord should be 0, "
               << "but is " << pcoords[i] << "\n";
          return 1;
        }
      }

      // validate the cellId
      ijk[i] = extent[2 * i + j];
      image->TransformContinuousIndexToPhysicalPoint(ijk, x);
      double pcoords2[3];
      int idx[3];
      if (image->ComputeStructuredCoordinates(x, idx, pcoords2) == 0)
      {
        cerr << "ComputeStructuredCoordinates failed for "
             << "point (" << x[0] << ", " << x[1] << ", " << x[2] << ")"
             << " and bounds (" << bounds[0] << ", " << bounds[1] << ", " << bounds[2] << ", "
             << bounds[3] << ", " << bounds[4] << ", " << bounds[5] << ")\n";
        return 1;
      }

      if (image->ComputeCellId(idx) != cellId)
      {
        cerr << "cellId = " << cellId << ", should be " << image->ComputeCellId(idx) << "\n";
        return 1;
      }

      // validate the pcoords, allow a tolerance
      double dist = pcoords[i] - pcoords2[i];
      if (dist * dist > 1e-29)
      {
        cerr << "pcoords[" << i << "] = " << pcoords[i] << ", should be " << pcoords2[i] << "\n";
        return 1;
      }
    }
  }

  return 0;
}

int TestImageDataFindCell(int, char*[])
{
  // test 0D, 1D, 2D, 3D data with various extents, spacings, origins
  static int dims[4][3] = { { 1, 1, 1 }, { 3, 1, 1 }, { 3, 3, 1 }, { 3, 3, 3 } };
  static int starts[4][3] = { { 0, 0, 0 }, { -1, 0, -1 }, { 2, 3, 6 }, { -10, 0, 5 } };
  static double spacings[4][3] = { { 1, 1, 1 }, { 1.0 / 7, 1, 1 }, { 1, -1, 1 },
    { -1, 1, -1 / 13.0 } };
  static double origins[4][3] = { { 0, 0, 0 }, { 1.0 / 13, 0, 0 }, { 0, -1, 0 },
    { -1, 0, -1 / 7.0 } };
  static double directions[4][9] = { { 1, 0, 0, 0, 1, 0, 0, 0, 1 }, { -1, 0, 0, 0, -1, 0, 0, 0, 1 },
    { 1, 0, 0, 0, 0, 1, 0, 1, 0 }, { 0, -1, 0, 1, 0, 0, 0, 0, 1 } };

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
            if (DoTest(extent, origin, spacing, direction))
            {
              failed = 1;
            }
          }
        }
      }
    }
  }

  return failed;
}
