/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPixel.cxx
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
#include "vtkPixel.h"
#include "vtkQuad.h"
#include "vtkTriangle.h"
#include "vtkPlane.h"
#include "vtkMath.h"
#include "vtkCellArray.h"
#include "vtkLine.h"
#include "vtkPointLocator.h"

// Construct the pixel with four points.
vtkPixel::vtkPixel()
{
  int i;
  
  this->Points->SetNumberOfPoints(4);
  this->PointIds->SetNumberOfIds(4);
  for (i = 0; i < 4; i++)
    {
    this->Points->SetPoint(i, 0.0, 0.0, 0.0);
    }
  for (i = 0; i < 4; i++)
    {
    this->PointIds->SetId(i,0);
    }
  this->Line = vtkLine::New();
}

vtkPixel::~vtkPixel()
{
  this->Line->Delete();
}

vtkCell *vtkPixel::MakeObject()
{
  vtkCell *cell = vtkPixel::New();
  cell->DeepCopy(this);
  return cell;
}

int vtkPixel::EvaluatePosition(float x[3], float* closestPoint,
                                  int& subId, float pcoords[3], 
                                  float& dist2, float *weights)
{
  float *pt1, *pt2, *pt3;
  int i;
  float p[3], p21[3], p31[3], cp[3];
  float l21, l31, n[3];

  subId = 0;
  pcoords[0] = pcoords[1] = pcoords[2] = 0.0;
//
// Get normal for pixel
//
  pt1 = this->Points->GetPoint(0);
  pt2 = this->Points->GetPoint(1);
  pt3 = this->Points->GetPoint(2);

  vtkTriangle::ComputeNormal (pt1, pt2, pt3, n);
//
// Project point to plane
//
  vtkPlane::ProjectPoint(x,pt1,n,cp);

  for (i=0; i<3; i++)
    {
    p21[i] = pt2[i] - pt1[i];
    p31[i] = pt3[i] - pt1[i];
    p[i] = x[i] - pt1[i];
    }

  if ( (l21=vtkMath::Norm(p21)) == 0.0 )
    {
    l21 = 1.0;
    }
  if ( (l31=vtkMath::Norm(p31)) == 0.0 )
    {
    l31 = 1.0;
    }

  pcoords[0] = vtkMath::Dot(p21,p) / (l21*l21);
  pcoords[1] = vtkMath::Dot(p31,p) / (l31*l31);

  this->InterpolationFunctions(pcoords, weights);

  if ( pcoords[0] >= 0.0 && pcoords[0] <= 1.0 &&
  pcoords[1] >= 0.0 && pcoords[1] <= 1.0 )
    {
    if (closestPoint)
      {
      closestPoint[0] = cp[0];
      closestPoint[1] = cp[1];
      closestPoint[2] = cp[2];
      dist2 = 
	vtkMath::Distance2BetweenPoints(closestPoint,x); //projection distance
      }
    return 1;
    }
  else
    {
    float pc[3], w[4];
    if (closestPoint)
      {
      for (i=0; i<2; i++)
	{
	if (pcoords[i] < 0.0)
	{
	pc[i] = 0.0;
	}
	else if (pcoords[i] > 1.0)
	  {
	  pc[i] = 1.0;
	  }
	else
	  {
	  pc[i] = pcoords[i];
	  }
	}
      this->EvaluateLocation(subId, pc, closestPoint, (float *)w);
      dist2 = vtkMath::Distance2BetweenPoints(closestPoint,x);
      }
    return 0;
    }
}

void vtkPixel::EvaluateLocation(int& subId, float pcoords[3], float x[3],
                                   float *weights)
{
  float *pt1, *pt2, *pt3;
  int i;

  subId = 0;
  
  pt1 = this->Points->GetPoint(0);
  pt2 = this->Points->GetPoint(1);
  pt3 = this->Points->GetPoint(2);

  for (i=0; i<3; i++)
    {
    x[i] = pt1[i] + pcoords[0]*(pt2[i] - pt1[i]) +
                    pcoords[1]*(pt3[i] - pt1[i]);
    }

  this->InterpolationFunctions(pcoords, weights);
}

int vtkPixel::CellBoundary(int vtkNotUsed(subId), float pcoords[3], vtkIdList *pts)
{
  float t1=pcoords[0]-pcoords[1];
  float t2=1.0-pcoords[0]-pcoords[1];

  pts->SetNumberOfIds(2);

  // compare against two lines in parametric space that divide element
  // into four pieces.
  if ( t1 >= 0.0 && t2 >= 0.0 )
    {
    pts->SetId(0,this->PointIds->GetId(0));
    pts->SetId(1,this->PointIds->GetId(1));
    }

  else if ( t1 >= 0.0 && t2 < 0.0 )
    {
    pts->SetId(0,this->PointIds->GetId(1));
    pts->SetId(1,this->PointIds->GetId(3));
    }

  else if ( t1 < 0.0 && t2 < 0.0 )
    {
    pts->SetId(0,this->PointIds->GetId(3));
    pts->SetId(1,this->PointIds->GetId(2));
    }

  else //( t1 < 0.0 && t2 >= 0.0 )
    {
    pts->SetId(0,this->PointIds->GetId(2));
    pts->SetId(1,this->PointIds->GetId(0));
    }

  if ( pcoords[0] < 0.0 || pcoords[0] > 1.0 ||
       pcoords[1] < 0.0 || pcoords[1] > 1.0 )
    {
    return 0;
    }
  else
    {
    return 1;
    }
}

//
// Marching squares
//
#include "vtkMarchingSquaresCases.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkPixel* vtkPixel::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPixel");
  if(ret)
    {
    return (vtkPixel*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPixel;
}




static int edges[4][2] = { {0,1}, {1,3}, {2,3}, {0,2} };

void vtkPixel::Contour(float value, vtkScalars *cellScalars,
		       vtkPointLocator *locator, 
		       vtkCellArray *vtkNotUsed(verts),
		       vtkCellArray *lines, 
		       vtkCellArray *vtkNotUsed(polys), 
                       vtkPointData *inPd, vtkPointData *outPd,
                       vtkCellData *inCd, int cellId, vtkCellData *outCd)
{
  static int CASE_MASK[4] = {1,2,8,4}; //note difference!
  LINE_CASES *lineCase;
  EDGE_LIST  *edge;
  int i, j, index, *vert;
  int newCellId;
  vtkIdType pts[2];
  float t, *x1, *x2, x[3];

  // Build the case table
  for ( i=0, index = 0; i < 4; i++)
    {
    if (cellScalars->GetScalar(i) >= value)
      {
      index |= CASE_MASK[i];
      }
    }

  lineCase = lineCases + index;
  edge = lineCase->edges;

  for ( ; edge[0] > -1; edge += 2 )
    {
    for (i=0; i<2; i++) // insert line
      {
      vert = edges[edge[i]];
      t = (value - cellScalars->GetScalar(vert[0])) /
          (cellScalars->GetScalar(vert[1]) - cellScalars->GetScalar(vert[0]));
      x1 = this->Points->GetPoint(vert[0]);
      x2 = this->Points->GetPoint(vert[1]);
      for (j=0; j<3; j++)
	{
	x[j] = x1[j] + t * (x2[j] - x1[j]);
	}
      if ( locator->InsertUniquePoint(x, pts[i]) )
        {
        if ( outPd ) 
          {
          int p1 = this->PointIds->GetId(vert[0]);
          int p2 = this->PointIds->GetId(vert[1]);
          outPd->InterpolateEdge(inPd,pts[i],p1,p2,t);
          }
        }
      }
    // check for degenerate line
    if ( pts[0] != pts[1] )
      {
      newCellId = lines->InsertNextCell(2,pts);
      outCd->CopyData(inCd,cellId,newCellId);
      }
    }
}

vtkCell *vtkPixel::GetEdge(int edgeId)
{
  int *verts;

  verts = edges[edgeId];

  // load point id's
  this->Line->PointIds->SetId(0,this->PointIds->GetId(verts[0]));
  this->Line->PointIds->SetId(1,this->PointIds->GetId(verts[1]));

  // load coordinates
  this->Line->Points->SetPoint(0,this->Points->GetPoint(verts[0]));
  this->Line->Points->SetPoint(1,this->Points->GetPoint(verts[1]));

  return this->Line;
}
//
// Compute interpolation functions (similar but different than Quad interpolation 
// functions)
//
void vtkPixel::InterpolationFunctions(float pcoords[3], float sf[4])
{
  float rm, sm;

  rm = 1. - pcoords[0];
  sm = 1. - pcoords[1];

  sf[0] = rm * sm;
  sf[1] = pcoords[0] * sm;
  sf[2] = rm * pcoords[1];
  sf[3] = pcoords[0] * pcoords[1];
}

//
// Compute derivatives of interpolation functions.
//
void vtkPixel::InterpolationDerivs(float pcoords[3], float derivs[8])
{
  double rm, sm;

  rm = 1. - pcoords[0];
  sm = 1. - pcoords[1];

  // r derivatives
  derivs[0] = -sm;
  derivs[1] = sm;
  derivs[2] = -pcoords[1];
  derivs[3] = pcoords[1];

  // s derivatives
  derivs[4] = -rm;
  derivs[5] = -pcoords[0];
  derivs[6] = rm;
  derivs[7] = pcoords[0];
}

// 
// Intersect plane; see whether point is inside.
//
int vtkPixel::IntersectWithLine(float p1[3], float p2[3], float tol, float& t,
                                float x[3], float pcoords[3], int& subId)
{
  float *pt1, *pt4, n[3];
  float tol2 = tol*tol;
  float closestPoint[3];
  float dist2, weights[4];
  int i;

  subId = 0;
  pcoords[0] = pcoords[1] = pcoords[2] = 0.0;
//
// Get normal for triangle
//
  pt1 = this->Points->GetPoint(0);
  pt4 = this->Points->GetPoint(3);

  n[0] = n[1] = n[2] = 0.0;
  for (i=0; i<3; i++)
    {
    if ( (pt4[i] - pt1[i]) <= 0.0 )
      {
      n[i] = 1.0;
      break;
      }
    }
  //
  // Intersect plane of pixel with line
  //
  if ( ! vtkPlane::IntersectWithLine(p1,p2,n,pt1,t,x) )
    {
    return 0;
    }
  //
  // Use evaluate position
  //
  if (this->EvaluatePosition(x, closestPoint, subId, pcoords, dist2, weights) )
    {
    if ( dist2 <= tol2 )
      {
      return 1;
      }
    }

  return 0;
}

int vtkPixel::Triangulate(int index, vtkIdList *ptIds, vtkPoints *pts)
{
  pts->Reset();
  ptIds->Reset();

  if ( (index % 2) )
    {
    ptIds->InsertId(0,this->PointIds->GetId(0));
    pts->InsertPoint(0,this->Points->GetPoint(0));
    ptIds->InsertId(1,this->PointIds->GetId(1));
    pts->InsertPoint(1,this->Points->GetPoint(1));
    ptIds->InsertId(2,this->PointIds->GetId(2));
    pts->InsertPoint(2,this->Points->GetPoint(2));

    ptIds->InsertId(3,this->PointIds->GetId(1));
    pts->InsertPoint(3,this->Points->GetPoint(1));
    ptIds->InsertId(4,this->PointIds->GetId(3));
    pts->InsertPoint(4,this->Points->GetPoint(3));
    ptIds->InsertId(5,this->PointIds->GetId(2));
    pts->InsertPoint(5,this->Points->GetPoint(2));
    }
  else
    {
    ptIds->InsertId(0,this->PointIds->GetId(0));
    pts->InsertPoint(0,this->Points->GetPoint(0));
    ptIds->InsertId(1,this->PointIds->GetId(1));
    pts->InsertPoint(1,this->Points->GetPoint(1));
    ptIds->InsertId(2,this->PointIds->GetId(3));
    pts->InsertPoint(2,this->Points->GetPoint(3));

    ptIds->InsertId(3,this->PointIds->GetId(1));
    pts->InsertPoint(3,this->Points->GetPoint(1));
    ptIds->InsertId(4,this->PointIds->GetId(3));
    pts->InsertPoint(4,this->Points->GetPoint(3));
    ptIds->InsertId(5,this->PointIds->GetId(0));
    pts->InsertPoint(5,this->Points->GetPoint(0));
    }

  return 1;
}

void vtkPixel::Derivatives(int vtkNotUsed(subId), 
			   float pcoords[3], 
			   float *values, 
			   int dim, float *derivs)
{
  float functionDerivs[8], sum;
  int i, j, k, plane, idx[2], jj;
  float *x0, *x1, *x2, *x3, spacing[3];

  x0 = this->Points->GetPoint(0);
  x1 = this->Points->GetPoint(1);
  x2 = this->Points->GetPoint(2);
  x3 = this->Points->GetPoint(3);

  //figure which plane this pixel is in
  for (i=0; i < 3; i++)
    {
    spacing[i] = x3[i] - x0[i];
    }

  if ( spacing[0] > spacing[2] && spacing[1] > spacing[2] ) // z-plane
    {
    plane = 2;
    idx[0] = 0; idx[1] = 1;
    }
  else if ( spacing[0] > spacing[1] && spacing[2] > spacing[1] ) // y-plane
    {
    plane = 1;
    idx[0] = 0; idx[1] = 2;
    }
  else // x-plane
    {
    plane = 0;
    idx[0] = 1; idx[1] = 2;
    }

  spacing[0] = x1[idx[0]] - x0[idx[0]];
  spacing[1] = x2[idx[1]] - x0[idx[1]];

  // get derivatives in r-s directions
  this->InterpolationDerivs(pcoords, functionDerivs);

  // since two of the x-y-z axes are aligned with r-s axes, only need to scale
  // the derivative values by the data spacing.
  for (k=0; k < dim; k++) //loop over values per vertex
    {
    for (jj=j=0; j < 3; j++) //loop over derivative directions
      {
      if ( j == plane ) // 0-derivate values in this direction
        {
        sum = 0.0;
        }
      else //compute derivatives
        {
        for (sum=0.0, i=0; i < 4; i++) //loop over interp. function derivatives
          {
          sum += functionDerivs[4*jj + i] * values[dim*i + k];
          }
        sum /= spacing[idx[jj++]];
        }
      derivs[3*k + j] = sum;
      }
    }
}

void vtkPixel::Clip(float vtkNotUsed(value), 
		    vtkScalars *vtkNotUsed(cellScalars), 
		    vtkPointLocator *vtkNotUsed(locator), 
		    vtkCellArray *vtkNotUsed(tetras),
		    vtkPointData *vtkNotUsed(inPd), vtkPointData *vtkNotUsed(outPd),
		    vtkCellData *vtkNotUsed(inCd), int vtkNotUsed(cellId), 
		    vtkCellData *vtkNotUsed(outCd), int vtkNotUsed(insideOut))
{

}

