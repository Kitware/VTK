/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEmptyCell.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkEmptyCell.h"
#include "vtkMath.h"
#include "vtkCellArray.h"
#include "vtkPointLocator.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkEmptyCell, "1.17");
vtkStandardNewMacro(vtkEmptyCell);

int vtkEmptyCell::EvaluatePosition(float  vtkNotUsed(x)[3], 
                                  float  vtkNotUsed(closestPoint)[3],
                                  int&   vtkNotUsed(subId), 
                                  float  vtkNotUsed(pcoords)[3], 
                                  float& vtkNotUsed(dist2), 
                                  float  *vtkNotUsed(weights))
{
  return 0;
}

void vtkEmptyCell::EvaluateLocation(int&  vtkNotUsed(subId), 
                                   float vtkNotUsed(pcoords)[3],
                                   float vtkNotUsed(x)[3],
                                   float *vtkNotUsed(weights))
{
}

int vtkEmptyCell::CellBoundary(int vtkNotUsed(subId), 
                            float vtkNotUsed(pcoords)[3], 
                            vtkIdList *vtkNotUsed(pts))
{
  return 0;
}

void vtkEmptyCell::Contour(float vtkNotUsed(value), 
                           vtkDataArray *vtkNotUsed(cellScalars), 
                           vtkPointLocator *vtkNotUsed(locator),
                           vtkCellArray *vtkNotUsed(verts), 
                           vtkCellArray *vtkNotUsed(lines), 
                           vtkCellArray *vtkNotUsed(polys), 
                           vtkPointData *vtkNotUsed(inPd),
                           vtkPointData *vtkNotUsed(outPd),
                           vtkCellData *vtkNotUsed(inCd),
                           vtkIdType vtkNotUsed(cellId), 
                           vtkCellData *vtkNotUsed(outCd))
{
}

// Project point on line. If it lies between 0<=t<=1 and distance off line
// is less than tolerance, intersection detected.
int vtkEmptyCell::IntersectWithLine(float vtkNotUsed(p1)[3], 
                                   float vtkNotUsed(p2)[3], 
                                   float vtkNotUsed(tol), 
                                   float& vtkNotUsed(t),
                                   float vtkNotUsed(x)[3], 
                                   float pcoords[3], 
                                   int& vtkNotUsed(subId))
{
  pcoords[0] = -10.0;
  return 0;
}

int vtkEmptyCell::Triangulate(int vtkNotUsed(index),
                             vtkIdList *ptIds, vtkPoints *pts)
{
  pts->Reset();
  ptIds->Reset();

  return 1;
}

void vtkEmptyCell::Derivatives(int vtkNotUsed(subId), 
                            float vtkNotUsed(pcoords)[3], 
                            float *vtkNotUsed(values), 
                            int vtkNotUsed(dim), 
                            float *vtkNotUsed(derivs))
{
}

void vtkEmptyCell::Clip(float vtkNotUsed(value), 
                        vtkDataArray *vtkNotUsed(cellScalars), 
                        vtkPointLocator *vtkNotUsed(locator), 
                        vtkCellArray *vtkNotUsed(verts),    
                        vtkPointData *vtkNotUsed(inPD),
                        vtkPointData *vtkNotUsed(outPD),
                        vtkCellData *vtkNotUsed(inCD), 
                        vtkIdType vtkNotUsed(cellId),
                        vtkCellData *vtkNotUsed(outCD),
                        int vtkNotUsed(insideOut))
{
}
