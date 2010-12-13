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
#include "vtkSmartPointer.h"

inline int DoTest(
  int extent[6], double origin[3], double spacing[3])
{
  vtkSmartPointer<vtkImageData> image =
    vtkSmartPointer<vtkImageData>::New();
  image->SetExtent(extent);
  image->SetOrigin(origin);
  image->SetSpacing(spacing);
  image->AllocateScalars();

  double bounds[6];
  image->GetBounds(bounds);

  int subId = 0;
  double pcoords[3];
  double weights[8];
  double x[3];
  vtkIdType cellId;

  double tol = 1e-4;

  for (int i = 0; i < 3; i++)
    {
    x[0] = 0.5*(bounds[0] + bounds[1]);
    x[1] = 0.5*(bounds[2] + bounds[3]);
    x[2] = 0.5*(bounds[4] + bounds[5]);
    for (int j = 0; j < 2; j++)
      {
      // test point right on the boundary with zero tolerance
      x[i] = bounds[2*i+j];

      cellId = image->FindCell(
        x, 0, 0, 0.0, subId, pcoords, weights);

      if (cellId < 0)
        {
        cerr << "point (" << x[0] << ", " << x[1] << ", " << x[2] << ")"
             << " should be in bounds (" << bounds[0] << ", " << bounds[1]
             << ", " << bounds[2] << ", " << bounds[3] << ", "
             << bounds[4] << ", " << bounds[5] << ") with tol 0.0\n";
        return 1;
        }

      // test point just outside boundary with zero tolerance
      double offset = ((j == 0) ? (-tol*0.5) : (tol*0.5));
      x[i] = bounds[2*i+j] + offset;

      cellId = image->FindCell(
        x, 0, 0, 0.0, subId, pcoords, weights);

      if (cellId >= 0)
        {
        cerr << "point (" << x[0] << ", " << x[1] << ", " << x[2] << ")"
             << " should be out of bounds (" << bounds[0] << ", " << bounds[1]
             << ", " << bounds[2] << ", " << bounds[3] << ", "
             << bounds[4] << ", " << bounds[5] << ") with tol 0.0\n";
        return 1;
        }

      // test point just outside boundary with nonzero tolerance
      x[i] = bounds[2*i+j] + ((j == 0) ? (-tol*0.5) : (tol*0.5));

      cellId = image->FindCell(
        x, 0, 0, tol*tol, subId, pcoords, weights);

      if (cellId < 0)
        {
        cerr << "point (" << x[0] << ", " << x[1] << ", " << x[2] << ")"
             << " should be inside bounds (" << bounds[0] << ", " << bounds[1]
             << ", " << bounds[2] << ", " << bounds[3] << ", "
             << bounds[4] << ", " << bounds[5] << ") with tol " << tol << "\n";
        return 1;
        }

      // check pcoords at boundaries
      int isUpperBound = ((j == 1) ^ (spacing[i] < 0));
      int isOnePixelThick = (extent[2*i] == extent[2*i+1]);
      if (isUpperBound && !isOnePixelThick)
        {
        if (pcoords[i] != 1.0)
          {
          cerr << "at upper bounds, pcoord should be 1, but is "
               << pcoords[i] << "\n";
          return 1;
          }
        }
      else
        {
        if (pcoords[i] != 0.0)
          {
          cerr << "at lower bounds and for 0,1,2D cells, pcoord should be 0, "
               << "but is " << pcoords[i] << " " << extent[2*i] << ", " << extent[2*i+1] << ", " << j << "\n";
          return 1;
          }
        }

      // validate the cellId
      x[i] = bounds[2*i+j];
      double pcoords2[3];
      int idx[3];
      if (image->ComputeStructuredCoordinates(x, idx, pcoords2) == 0)
        {
        cerr << "ComputeStructuredCoordinates failed for "
             << "point (" << x[0] << ", " << x[1] << ", " << x[2] << ")"
             << " and bounds (" << bounds[0] << ", " << bounds[1]
             << ", " << bounds[2] << ", " << bounds[3] << ", "
             << bounds[4] << ", " << bounds[5] << ")\n";
        return 1;
        }

      if (image->ComputeCellId(idx) != cellId)
        {
        cerr << "cellId = " << cellId << ", should be "
             << image->ComputeCellId(idx) << "\n";
        return 1;
        }

      // validate the pcoords, allow a tolerance
      double diff = (pcoords[i] - pcoords2[i]);
      if (diff*diff > (1e-15)*(1e-15))
        {
        cerr << "pcoords[" << i << "] = " << pcoords[i] << ", should be "
             << pcoords2[i] << "\n";
        return 1;
        }
      }
    }

  return 0;
}


int TestImageDataFindCell(int,char *[])
{
  // test 0D, 1D, 2D, 3D data with various extents, spacings, origins
  static int dims[4][3] = {
    { 1, 1, 1 }, { 3, 1, 1 }, { 3, 3, 1 }, { 3, 3, 3 } };
  static int starts[4][3] = {
    { 0, 0, 0 }, { -1, 0, -1 }, { 2, 3, 6 }, { -10, 0, 5 } };
  static double spacings[4][3] = {
    { 1, 1, 1 }, { 1.0/7, 1, 1 }, { 1, -1, 1 }, { -1, 1, -1/13.0 } };
  static double origins[4][3] = {
    { 0, 0, 0 }, { 1.0/13, 0, 0 }, { 0, -1, 0 }, { -1, 0, -1/7.0 } };

  int extent[6];
  double *spacing;
  double *origin;

  int failed = 0;

  for (int i = 0; i < 4; i++)
    {
    for (int j = 0; j < 4; j++)
      {
      for (int k = 0; k < 4; k++)
        {
        for (int l = 0; l < 4; l++)
          {
          for (int ii = 0; ii < 3; ii++)
            {
            extent[2*ii] = starts[i][ii];
            extent[2*ii+1] = starts[i][ii] + dims[j][ii] - 1;
            }
          spacing = spacings[k];
          origin = origins[l];

          if (DoTest(extent, origin, spacing))
            {
            failed = 1;
            }
          }
        }
      }
    }

  return failed;
}
