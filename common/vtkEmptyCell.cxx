/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEmptyCell.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkEmptyCell.h"
#include "vtkMath.h"
#include "vtkCellArray.h"
#include "vtkPointLocator.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkEmptyCell* vtkEmptyCell::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkEmptyCell");
  if(ret)
    {
    return (vtkEmptyCell*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkEmptyCell;
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
			    vtkIdList *vtkNotUsed(pts))
{
  return 0;
}

void vtkEmptyCell::Contour(float vtkNotUsed(value), 
			   vtkScalars *vtkNotUsed(cellScalars), 
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
			vtkScalars *vtkNotUsed(cellScalars), 
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
