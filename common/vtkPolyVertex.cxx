/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyVertex.cxx
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
#include "vtkPolyVertex.h"
#include "vtkMath.h"
#include "vtkCellArray.h"
#include "vtkPointLocator.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkPolyVertex* vtkPolyVertex::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPolyVertex");
  if(ret)
    {
    return (vtkPolyVertex*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPolyVertex;
}





vtkPolyVertex::vtkPolyVertex()
{
  this->Vertex = vtkVertex::New();
}

vtkPolyVertex::~vtkPolyVertex()
{
  this->Vertex->Delete();
}

vtkCell *vtkPolyVertex::MakeObject()
{
  vtkCell *cell = vtkPolyVertex::New();
  cell->DeepCopy(this);
  return cell;
}

int vtkPolyVertex::EvaluatePosition(float x[3], float* closestPoint,
                                   int& subId, float pcoords[3], 
                                   float& minDist2, float *weights)
{
  int numPts=this->Points->GetNumberOfPoints();
  float *X;
  float dist2;
  int i;

  for (minDist2=VTK_LARGE_FLOAT, i=0; i<numPts; i++)
    {
    X = this->Points->GetPoint(i);
    dist2 = vtkMath::Distance2BetweenPoints(X,x);
    if (dist2 < minDist2)
      {
      if (closestPoint)
	{
	closestPoint[0] = X[0]; closestPoint[1] = X[1]; closestPoint[2] = X[2];
	}
      minDist2 = dist2;
      subId = i;
      }
    }

  for (i=0; i<numPts; i++)
    {
    weights[i] = 0.0;
    }
  weights[subId] = 1.0;

  if (minDist2 == 0.0)
    {
    pcoords[0] = 0.0;
    return 1;
    }
  else
    {
    pcoords[0] = -10.0;
    return 0;
    }
}

void vtkPolyVertex::EvaluateLocation(int& subId, 
				     float vtkNotUsed(pcoords)[3], 
                                     float x[3], float *weights)
{
  int i;
  float *X = this->Points->GetPoint(subId);
  x[0] = X[0];
  x[1] = X[1];
  x[2] = X[2];

  for (i=0; i<this->GetNumberOfPoints(); i++)
    {
    weights[i] = 0.0;
    }
  weights[subId] = 1.0;
}

int vtkPolyVertex::CellBoundary(int subId, float pcoords[3], vtkIdList *pts)
{
  pts->SetNumberOfIds(1);
  pts->SetId(0,this->PointIds->GetId(subId));

  if ( pcoords[0] != 0.0 )
    {
    return 0;
    }
  else
    {
    return 1;
    }
}

void vtkPolyVertex::Contour(float value, vtkScalars *cellScalars, 
			    vtkPointLocator *locator, vtkCellArray *verts,
			    vtkCellArray *vtkNotUsed(lines), 
			    vtkCellArray *vtkNotUsed(polys), 
                            vtkPointData *inPd, vtkPointData *outPd,
                            vtkCellData *inCd, int cellId, vtkCellData *outCd)
{
  int i, numPts=this->Points->GetNumberOfPoints(), newCellId;
  vtkIdType pts[1];
  
  for (i=0; i < numPts; i++)
    {
    if ( value == cellScalars->GetScalar(i) )
      {
      pts[0] = locator->InsertNextPoint(this->Points->GetPoint(i));
      if ( outPd )
        {   
        outPd->CopyData(inPd,this->PointIds->GetId(i),pts[0]);
        }
      newCellId = verts->InsertNextCell(1,pts);
      outCd->CopyData(inCd,cellId,newCellId);
      }
    }
}

//
// Intersect with sub-vertices
//
int vtkPolyVertex::IntersectWithLine(float p1[3], float p2[3], 
                                    float tol, float& t, float x[3], 
                                    float pcoords[3], int& subId)
{
  int subTest, numPts=this->Points->GetNumberOfPoints();

  for (subId=0; subId < numPts; subId++)
    {
    this->Vertex->Points->SetPoint(0,this->Points->GetPoint(subId));

    if ( this->Vertex->IntersectWithLine(p1, p2, tol, t, x, pcoords, subTest) )
      {
      return 1;
      }
    }

  return 0;
}

int vtkPolyVertex::Triangulate(int vtkNotUsed(index), vtkIdList *ptIds, 
                               vtkPoints *pts)
{
  int subId;

  pts->Reset();
  ptIds->Reset();
  for (subId=0; subId < this->Points->GetNumberOfPoints(); subId++)
    {
    pts->InsertPoint(subId,this->Points->GetPoint(subId));
    ptIds->InsertId(subId,this->PointIds->GetId(subId));
    }
  return 1;
}

void vtkPolyVertex::Derivatives(int vtkNotUsed(subId), 
				float vtkNotUsed(pcoords)[3], 
				float *vtkNotUsed(values), 
				int dim, float *derivs)
{
  int i, idx;

  for (i=0; i<dim; i++)
    {
    idx = i*dim;
    derivs[idx] = 0.0;
    derivs[idx+1] = 0.0;
    derivs[idx+2] = 0.0;
    }
}

void vtkPolyVertex::Clip(float value, vtkScalars *cellScalars, 
                         vtkPointLocator *locator, vtkCellArray *verts,
                         vtkPointData *inPd, vtkPointData *outPd,
                         vtkCellData *inCd, int cellId, vtkCellData *outCd,
                         int insideOut)
{
  float s, x[3];
  int i, newCellId, numPts=this->Points->GetNumberOfPoints();
  vtkIdType pts[1];
  
  for ( i=0; i < numPts; i++ )
    {
    s = cellScalars->GetScalar(i);

    if ( (!insideOut && s > value) || (insideOut && s <= value) )
      {
      this->Points->GetPoint(i,x);
      if ( locator->InsertUniquePoint(x, pts[0]) )
        {
        outPd->CopyData(inPd,this->PointIds->GetId(i),pts[0]);
        }
      newCellId = verts->InsertNextCell(1,pts);
      outCd->CopyData(inCd,cellId,newCellId);
      }
    }
}

// Return the center of the point cloud in parametric coordinates.
int vtkPolyVertex::GetParametricCenter(float pcoords[3])
{
  pcoords[0] = pcoords[1] = pcoords[2] = 0.5;
  return (this->Points->GetNumberOfPoints() / 2);
}

