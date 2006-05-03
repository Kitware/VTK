/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGenericCell.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGenericCell.h"
#include "vtkMath.h"

int TestGenericCell(int , char *[])
{
  int rval = 0;
  vtkGenericCell *cell = vtkGenericCell::New();

  for(int i=0; i<VTK_NUMBER_OF_CELL_TYPES;++i)
    {
    cell->SetCellType( i );
    if( cell->RequiresInitialization() )
      {
      cell->Initialize();
      }
    cell->Print( cout );
    int numPts   = cell->GetNumberOfPoints();
    int numEdges = cell->GetNumberOfEdges();
    int numFaces = cell->GetNumberOfFaces();
    double center[3];
    int a = cell->GetParametricCenter(center);
    (void)a;
    double *pcoords = cell->GetParametricCoords();
    if( cell->GetCellType() != VTK_EMPTY_CELL 
     && cell->GetCellType() != VTK_POLY_VERTEX // FIXME
     && cell->GetCellType() != VTK_POLY_LINE // FIXME
     && cell->GetCellType() != VTK_TRIANGLE_STRIP // FIXME
     && cell->GetCellType() != VTK_POLYGON // FIXME
     && cell->GetCellType() != VTK_CONVEX_POINT_SET) // FIXME
      {
      double m[3] = {0., 0., 0.};
      // We add all the points since
      // Those on the corner points indeed define the parametric center
      // The dof node (center mid points) by definition have the same parametric center
      // and taking into account the center point only add a 0 vector to the sum
      // therefore we do not need to differenciate corner from the rest in this sum:
      for(int j=0; j<numPts; ++j)
        {
        double *point = pcoords + 3*j;
        m[0] += point[0];
        m[1] += point[1];
        m[2] += point[2];
        }
      m[0] /= numPts;
      m[1] /= numPts;
      m[2] /= numPts;
      if( fabs( center[0] - m[0] ) > 1e-6
       || fabs( center[1] - m[1] ) > 1e-6
       || fabs( center[2] - m[2] ) > 1e-6)
        {
        cerr << "Cell: " << i << endl;
        cerr << "Center: " << center[0] << "," << center[1] << "," << center[2] << endl;
        cerr << "M     : " << m[0] << "," << m[1] << "," << m[2] << endl;
        ++rval;
        }
      }
    int p = cell->IsPrimaryCell();
    (void)p;
    int cellDim = cell->GetCellDimension();
    (void)cellDim;
    int l = cell->IsLinear();
    (void)l;

    for(int e=0; e<numEdges; ++e)
      {
      vtkCell *c = cell->GetEdge(e);
      c->Print( cout );
      }
    for(int f=0; f<numFaces; ++f)
      {
      vtkCell *c = cell->GetFace(f);
      c->Print( cout );
      }
    if( cell->GetCellType() != i && cell->GetCellType() != VTK_EMPTY_CELL )
      {
      ++rval;
      }
    }

  cell->Delete();

  return rval;
}


