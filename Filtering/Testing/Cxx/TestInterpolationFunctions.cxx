/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestInterpolationFunctions.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMath.h" // for VTK_DBL_EPSILON

// Subclass of vtkCell3D
//#include "vtkConvexPointSet.h"
#include "vtkHexagonalPrism.h"
#include "vtkHexahedron.h"
#include "vtkPentagonalPrism.h"
#include "vtkPyramid.h"
#include "vtkTetra.h"
#include "vtkVoxel.h"
#include "vtkWedge.h"

template <class TCell>
int TestOneInterpolationFunction()
{
  TCell *cell = TCell::New();
  int numPts = cell->GetNumberOfPoints();
  double *sf = new double[numPts];
  double *coords = cell->GetParametricCoords();
  cell->Delete();
  int r = 0;
  for(int i=0;i<numPts;)
    {
    double point[3];
    point[0] = coords[i++];
    point[1] = coords[i++];
    point[2] = coords[i];
    TCell::InterpolationFunctions(point, sf); // static function
    for(int j=0;j<numPts/3;j++)
      {
      if(j == (i/3))
        {
        if( fabs(sf[j] - 1) > VTK_DBL_EPSILON)
          {
          ++r;
          }
        }
      else
        {
        if( fabs(sf[j] - 0) > VTK_DBL_EPSILON )
          {
          ++r;
          }
        }
      }
    ++i;
    }

  delete[] sf;
  return r;
}

int TestInterpolationFunctions(int, char *[])
{
  int r = 0;

  // Subclass of vtkCell3D
  //r += TestOneInterpolationFunction<vtkConvexPointSet>(); // not implemented
  r += TestOneInterpolationFunction<vtkHexagonalPrism>();
  r += TestOneInterpolationFunction<vtkHexahedron>();
  r += TestOneInterpolationFunction<vtkPentagonalPrism>();
  r += TestOneInterpolationFunction<vtkPyramid>();
  r += TestOneInterpolationFunction<vtkTetra>();
  r += TestOneInterpolationFunction<vtkVoxel>();
  r += TestOneInterpolationFunction<vtkWedge>();

  return r;
}
