/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEmptyCell.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkEmptyCell.h"

#include "vtkCellArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"

vtkStandardNewMacro(vtkEmptyCell);

//----------------------------------------------------------------------------
int vtkEmptyCell::EvaluatePosition(double  vtkNotUsed(x)[3],
                                  double  vtkNotUsed(closestPoint)[3],
                                  int&   vtkNotUsed(subId),
                                  double  vtkNotUsed(pcoords)[3],
                                  double& vtkNotUsed(dist2),
                                  double  *vtkNotUsed(weights))
{
  return 0;
}

//----------------------------------------------------------------------------
void vtkEmptyCell::EvaluateLocation(int&  vtkNotUsed(subId),
                                   double vtkNotUsed(pcoords)[3],
                                   double vtkNotUsed(x)[3],
                                   double *vtkNotUsed(weights))
{
}

//----------------------------------------------------------------------------
int vtkEmptyCell::CellBoundary(int vtkNotUsed(subId),
                            double vtkNotUsed(pcoords)[3],
                            vtkIdList *vtkNotUsed(pts))
{
  return 0;
}

//----------------------------------------------------------------------------
void vtkEmptyCell::Contour(double vtkNotUsed(value),
                           vtkDataArray *vtkNotUsed(cellScalars),
                           vtkIncrementalPointLocator *vtkNotUsed(locator),
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

//----------------------------------------------------------------------------
// Project point on line. If it lies between 0<=t<=1 and distance off line
// is less than tolerance, intersection detected.
int vtkEmptyCell::IntersectWithLine(double vtkNotUsed(p1)[3],
                                   double vtkNotUsed(p2)[3],
                                   double vtkNotUsed(tol),
                                   double& vtkNotUsed(t),
                                   double vtkNotUsed(x)[3],
                                   double pcoords[3],
                                   int& vtkNotUsed(subId))
{
  pcoords[0] = -10.0;
  return 0;
}

//----------------------------------------------------------------------------
int vtkEmptyCell::Triangulate(int vtkNotUsed(index),
                             vtkIdList *ptIds, vtkPoints *pts)
{
  pts->Reset();
  ptIds->Reset();

  return 1;
}

//----------------------------------------------------------------------------
void vtkEmptyCell::Derivatives(int vtkNotUsed(subId),
                            double vtkNotUsed(pcoords)[3],
                            double *vtkNotUsed(values),
                            int vtkNotUsed(dim),
                            double *vtkNotUsed(derivs))
{
}

//----------------------------------------------------------------------------
void vtkEmptyCell::Clip(double vtkNotUsed(value),
                        vtkDataArray *vtkNotUsed(cellScalars),
                        vtkIncrementalPointLocator *vtkNotUsed(locator),
                        vtkCellArray *vtkNotUsed(verts),
                        vtkPointData *vtkNotUsed(inPD),
                        vtkPointData *vtkNotUsed(outPD),
                        vtkCellData *vtkNotUsed(inCD),
                        vtkIdType vtkNotUsed(cellId),
                        vtkCellData *vtkNotUsed(outCD),
                        int vtkNotUsed(insideOut))
{
}

//----------------------------------------------------------------------------
void vtkEmptyCell::InterpolateFunctions(double pcoords[3], double *weights)
{
  (void)pcoords;
  (void)weights;
}

//----------------------------------------------------------------------------
void vtkEmptyCell::InterpolateDerivs(double pcoords[3], double *derivs)
{
  (void)pcoords;
  (void)derivs;
}

//----------------------------------------------------------------------------
void vtkEmptyCell::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
