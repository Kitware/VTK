/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEmptyCell.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkEmptyCell.h"
#include "vtkMath.h"
#include "vtkCellArray.h"
#include "vtkPointLocator.h"

// Description:
// Deep copy of cell.
vtkEmptyCell::vtkEmptyCell(const vtkEmptyCell& vtkNotUsed(p))
{
}

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
			    vtkIdList& vtkNotUsed(pts))
{
  return 0;
}

void vtkEmptyCell::Contour(float vtkNotUsed(value), 
                          vtkFloatScalars *vtkNotUsed(cellScalars), 
                          vtkPointLocator *vtkNotUsed(locator),
                          vtkCellArray *vtkNotUsed(verts), 
                          vtkCellArray *vtkNotUsed(lines), 
                          vtkCellArray *vtkNotUsed(polys), 
                          vtkPointData *vtkNotUsed(inPd), 
                          vtkPointData *vtkNotUsed(outPd))
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
                             vtkIdList& ptIds, vtkFloatPoints& pts)
{
  pts.Reset();
  ptIds.Reset();

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
                       vtkFloatScalars *vtkNotUsed(cellScalars), 
                       vtkPointLocator *vtkNotUsed(locator), 
                       vtkCellArray *vtkNotUsed(verts),    
                       vtkPointData *vtkNotUsed(inPD), 
                       vtkPointData *vtkNotUsed(outPD),
                       int vtkNotUsed(insideOut))
{
}
