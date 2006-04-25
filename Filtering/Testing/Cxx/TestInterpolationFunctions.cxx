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
#define VTK_EPSILON 1e-10

// Subclass of vtkCell
//#include "vtkEmptyCell.h"
#include "vtkGenericCell.h"
#include "vtkLine.h"
#include "vtkPixel.h"
//#include "vtkPolygon.h"
//#include "vtkPolyLine.h"
//#include "vtkPolyVertex.h"
#include "vtkQuad.h"
#include "vtkTriangle.h"
//#include "vtkTriangleStrip.h"
#include "vtkVertex.h"

// Subclass of vtkCell3D
//#include "vtkConvexPointSet.h"
#include "vtkHexagonalPrism.h"
#include "vtkHexahedron.h"
#include "vtkPentagonalPrism.h"
#include "vtkPyramid.h"
#include "vtkTetra.h"
#include "vtkVoxel.h"
#include "vtkWedge.h"

// Subclass of vtkNonLinearCell
//#include "vtkExplicitCell.h"
#include "vtkQuadraticEdge.h"
#include "vtkQuadraticHexahedron.h"
#include "vtkQuadraticPyramid.h"
#include "vtkQuadraticQuad.h"
#include "vtkQuadraticTetra.h"
#include "vtkQuadraticTriangle.h"
#include "vtkQuadraticWedge.h"

// Bi/Tri linear quadratic cells
#include "vtkBiQuadraticQuad.h"
#include "vtkBiQuadraticQuadraticHexahedron.h"
#include "vtkBiQuadraticQuadraticWedge.h"
#include "vtkQuadraticLinearQuad.h"
#include "vtkQuadraticLinearWedge.h"
#include "vtkTriQuadraticHexahedron.h"

template <class TCell>
int TestOneInterpolationFunction()
{
  TCell *cell = TCell::New();
  int numPts = cell->GetNumberOfPoints();
  double *sf = new double[numPts];
  double *coords = cell->GetParametricCoords();
  int r = 0;
  for(int i=0;i<numPts*3;)
    {
    double point[3];
    point[0] = coords[i++];
    point[1] = coords[i++];
    point[2] = coords[i];
    double sum = 0.;
    TCell::InterpolationFunctions(point, sf); // static function
    for(int j=0;j<numPts;j++)
      {
      sum += sf[j];
      if(j == (i/3))
        {
        if( fabs(sf[j] - 1) > VTK_EPSILON)
          {
          ++r;
          }
        }
      else
        {
        if( fabs(sf[j] - 0) > VTK_EPSILON )
          {
          ++r;
          }
        }
      }
    if( fabs(sum - 1) > VTK_EPSILON )
      {
      ++r;
      }
    ++i;
    }

  // Let's test unity condition on the center point:
  double center[3];
  cell->GetParametricCenter(center);
  TCell::InterpolationFunctions(center, sf); // static function
  double sum = 0.;
  for(int j=0;j<numPts;j++)
    {
    sum += sf[j];
    }
  if( !cell->IsA( "vtkPentagonalPrism" ) )
    {
    if( fabs(sum - 1) > VTK_EPSILON )
      {
      ++r;
      }
    }

  cell->Delete();
  delete[] sf;
  return r;
}

int TestInterpolationFunctions(int, char *[])
{
  int r = 0;
  // Subclass of vtkCell3D
  //r += TestOneInterpolationFunction<vtkEmptyCell>(); // not implemented
  //r += TestOneInterpolationFunction<vtkGenericCell>(); // not implemented
  r += TestOneInterpolationFunction<vtkLine>();
  r += TestOneInterpolationFunction<vtkPixel>();
  //r += TestOneInterpolationFunction<vtkPolygon>(); // not implemented
  //r += TestOneInterpolationFunction<vtkPolyLine>(); // not implemented
  //r += TestOneInterpolationFunction<vtkPolyVertex>(); // not implemented
  r += TestOneInterpolationFunction<vtkQuad>();
  r += TestOneInterpolationFunction<vtkTriangle>();
  //r += TestOneInterpolationFunction<vtkTriangleStrip>(); // not implemented
  r += TestOneInterpolationFunction<vtkVertex>();

  // Subclass of vtkCell3D
  //r += TestOneInterpolationFunction<vtkConvexPointSet>(); // not implemented
  r += TestOneInterpolationFunction<vtkHexagonalPrism>();
  r += TestOneInterpolationFunction<vtkHexahedron>();
  r += TestOneInterpolationFunction<vtkPentagonalPrism>();
  r += TestOneInterpolationFunction<vtkPyramid>();
  r += TestOneInterpolationFunction<vtkTetra>();
  r += TestOneInterpolationFunction<vtkVoxel>();
  r += TestOneInterpolationFunction<vtkWedge>();

  // Subclass of vtkNonLinearCell
  //r += TestOneInterpolationFunction<vtkExplicitCell>(); // not implemented
  r += TestOneInterpolationFunction<vtkQuadraticEdge>();
  r += TestOneInterpolationFunction<vtkQuadraticHexahedron>();
  r += TestOneInterpolationFunction<vtkQuadraticPyramid>();
  r += TestOneInterpolationFunction<vtkQuadraticQuad>();
  r += TestOneInterpolationFunction<vtkQuadraticTetra>();
  r += TestOneInterpolationFunction<vtkQuadraticTriangle>();
  r += TestOneInterpolationFunction<vtkQuadraticWedge>();

  // Bi/Tri linear quadratic cells
  r += TestOneInterpolationFunction<vtkBiQuadraticQuad>();
  r += TestOneInterpolationFunction<vtkBiQuadraticQuadraticHexahedron>();
  r += TestOneInterpolationFunction<vtkBiQuadraticQuadraticWedge>();
  r += TestOneInterpolationFunction<vtkQuadraticLinearQuad>();
  r += TestOneInterpolationFunction<vtkQuadraticLinearWedge>();
  r += TestOneInterpolationFunction<vtkTriQuadraticHexahedron>();

  return r;
}
