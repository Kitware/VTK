/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTriangleStrip.cxx
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
#include "vtkTriangleStrip.h"
#include "vtkCellArray.h"
#include "vtkLine.h"
#include "vtkObjectFactory.h"
#include "vtkFloatArray.h"


//------------------------------------------------------------------------------
vtkTriangleStrip* vtkTriangleStrip::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkTriangleStrip");
  if(ret)
    {
    return (vtkTriangleStrip*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkTriangleStrip;
}





vtkTriangleStrip::vtkTriangleStrip()
{
  this->Line = vtkLine::New();
  this->Triangle = vtkTriangle::New();
}

vtkTriangleStrip::~vtkTriangleStrip()
{
  this->Line->Delete();
  this->Triangle->Delete();
}

vtkCell *vtkTriangleStrip::MakeObject()
{
  vtkCell *cell = vtkTriangleStrip::New();
  cell->DeepCopy(this);
  return cell;
}

int vtkTriangleStrip::EvaluatePosition(float x[3], float* closestPoint,
                                      int& subId, float pcoords[3], 
                                      float& minDist2, float *weights)
{
  float pc[3], dist2;
  int ignoreId, i, return_status, status;
  float tempWeights[3], activeWeights[3];
  float closest[3];

  pcoords[2] = 0.0;

  return_status = 0;
  for (minDist2=VTK_LARGE_FLOAT,i=0; i<this->Points->GetNumberOfPoints()-2; i++)
    {
    weights[i] = 0.0;
    this->Triangle->Points->SetPoint(0,this->Points->GetPoint(i));
    this->Triangle->Points->SetPoint(1,this->Points->GetPoint(i+1));
    this->Triangle->Points->SetPoint(2,this->Points->GetPoint(i+2));
    status = this->Triangle->EvaluatePosition(x,closest,ignoreId,pc,dist2,tempWeights);
    if ( status != -1 && dist2 < minDist2 )
      {
      return_status = status;
      if (closestPoint)
	{
	closestPoint[0] = closest[0]; 
	closestPoint[1] = closest[1]; 
	closestPoint[2] = closest[2];
	}
      subId = i;
      pcoords[0] = pc[0];
      pcoords[1] = pc[1];
      pcoords[2] = 1.0 - pc[0] - pc[1];
      minDist2 = dist2;
      activeWeights[0] = tempWeights[0];
      activeWeights[1] = tempWeights[1];
      activeWeights[2] = tempWeights[2];
      }
    }

  weights[i] = 0.0;  
  weights[i+1] = 0.0;  

  weights[subId] = activeWeights[0];
  weights[subId+1] = activeWeights[1];
  weights[subId+2] = activeWeights[2];

  return return_status;
}

void vtkTriangleStrip::EvaluateLocation(int& subId, float pcoords[3], 
                                        float x[3], float *weights)
{
  int i;
  static int idx[2][3]={{0,1,2},{1,0,2}};
  int order = subId % 2;

  float *pt1 = this->Points->GetPoint(subId+idx[order][0]);
  float *pt2 = this->Points->GetPoint(subId+idx[order][1]);
  float *pt3 = this->Points->GetPoint(subId+idx[order][2]);
  float u3 = 1.0 - pcoords[0] - pcoords[1];

  weights[0] = u3;
  weights[1] = pcoords[0];
  weights[2] = pcoords[1];

  for (i=0; i<3; i++)
    {
    x[i] = pt1[i]*weights[0] + pt2[i]*weights[1] + pt3[i]*weights[2];
    }
}

int vtkTriangleStrip::CellBoundary(int subId, float pcoords[3], vtkIdList *pts)
{
  static int idx[2][3]={{0,1,2},{1,0,2}};
  int order;

  order = subId % 2;

  this->Triangle->PointIds->SetId(0,this->PointIds->GetId(subId + idx[order][0]));
  this->Triangle->PointIds->SetId(1,this->PointIds->GetId(subId + idx[order][1]));
  this->Triangle->PointIds->SetId(2,this->PointIds->GetId(subId + idx[order][2]));
  return this->Triangle->CellBoundary(0, pcoords, pts);
}

void vtkTriangleStrip::Contour(float value, vtkDataArray *cellScalars, 
                               vtkPointLocator *locator, vtkCellArray *verts, 
                               vtkCellArray *lines, vtkCellArray *polys, 
                               vtkPointData *inPd, vtkPointData *outPd,
                               vtkCellData *inCd, vtkIdType cellId,
                               vtkCellData *outCd)
{
  int i, numTris=this->Points->GetNumberOfPoints()-2;
  vtkDataArray *triScalars=cellScalars->MakeObject();
  triScalars->SetNumberOfTuples(3);

  for ( i=0; i < numTris; i++)
    {
    this->Triangle->Points->SetPoint(0,this->Points->GetPoint(i));
    this->Triangle->Points->SetPoint(1,this->Points->GetPoint(i+1));
    this->Triangle->Points->SetPoint(2,this->Points->GetPoint(i+2));

    if ( outPd )
      {
      this->Triangle->PointIds->SetId(0,this->PointIds->GetId(i));
      this->Triangle->PointIds->SetId(1,this->PointIds->GetId(i+1));
      this->Triangle->PointIds->SetId(2,this->PointIds->GetId(i+2));
      }

    triScalars->SetTuple(0,cellScalars->GetTuple(i));
    triScalars->SetTuple(1,cellScalars->GetTuple(i+1));
    triScalars->SetTuple(2,cellScalars->GetTuple(i+2));

    this->Triangle->Contour(value, triScalars, locator, verts,
			   lines, polys, inPd, outPd, inCd, cellId, outCd);
    }
  triScalars->Delete();
}


vtkCell *vtkTriangleStrip::GetEdge(int edgeId)
{
  int id1, id2;

  if ( edgeId == 0 )
    {
    id1 = 0;
    id2 = 1;
    }
  else if ( edgeId == (this->GetNumberOfPoints()-1) )
    {
    id1 = edgeId - 1;
    id2 = edgeId;
    }
  else
    {
    id1 = edgeId - 1;
    id2 = edgeId + 1;
    }

  this->Line->PointIds->SetId(0,this->PointIds->GetId(id1));
  this->Line->PointIds->SetId(1,this->PointIds->GetId(id2));
  this->Line->Points->SetPoint(0,this->Points->GetPoint(id1));
  this->Line->Points->SetPoint(1,this->Points->GetPoint(id2));

  return this->Line;
}

// 
// Intersect sub-triangles
//
int vtkTriangleStrip::IntersectWithLine(float p1[3], float p2[3], float tol,
                                       float& t, float x[3], float pcoords[3],
                                       int& subId)
{
  int subTest, numTris=this->Points->GetNumberOfPoints()-2;

  for (subId=0; subId < numTris; subId++)
    {
    this->Triangle->Points->SetPoint(0,this->Points->GetPoint(subId));
    this->Triangle->Points->SetPoint(1,this->Points->GetPoint(subId+1));
    this->Triangle->Points->SetPoint(2,this->Points->GetPoint(subId+2));

    if (this->Triangle->IntersectWithLine(p1, p2, tol, t, x, pcoords, subTest) )
      {
      return 1;
      }
    }

  return 0;
}

int vtkTriangleStrip::Triangulate(int vtkNotUsed(index), vtkIdList *ptIds, 
                                  vtkPoints *pts)
{
  int numTris = this->Points->GetNumberOfPoints()-2;
  int i, order;
  static int idx[2][3]={{0,1,2},{1,0,2}};

  pts->Reset();
  ptIds->Reset();

  for (int subId=0; subId < numTris; subId++)
    {
    order = subId % 2;

    for ( i=0; i < 3; i++ )
      {
      ptIds->InsertNextId(this->PointIds->GetId(subId+idx[order][i]));
      pts->InsertNextPoint(this->Points->GetPoint(subId+idx[order][i]));
      }
    }

  return 1;
}

void vtkTriangleStrip::Derivatives(int subId, float pcoords[3], float *values, 
                                   int dim, float *derivs)
{
  this->Triangle->Points->SetPoint(0,this->Points->GetPoint(subId));
  this->Triangle->Points->SetPoint(1,this->Points->GetPoint(subId+1));
  this->Triangle->Points->SetPoint(2,this->Points->GetPoint(subId+2));

  this->Triangle->Derivatives(0, pcoords, values+dim*subId, dim, derivs);
}

// Given a triangle strip, decompose it into (triangle) polygons. The 
// polygons are appended to the end of the list of polygons.
void vtkTriangleStrip::DecomposeStrip(int npts, vtkIdType *pts,
                                      vtkCellArray *polys)
{
  int p1, p2, p3, i;

  p1 = pts[0];
  p2 = pts[1];
  for (i=0; i<(npts-2); i++)
    {
    p3 = pts[i+2];
    polys->InsertNextCell(3);
    if ( (i % 2) ) // flip ordering to preserve consistency
      {
      polys->InsertCellPoint(p2);
      polys->InsertCellPoint(p1);
      polys->InsertCellPoint(p3);
      }
    else
      {
      polys->InsertCellPoint(p1);
      polys->InsertCellPoint(p2);
      polys->InsertCellPoint(p3);
      }
    p1 = p2;
    p2 = p3;
    }
}

void vtkTriangleStrip::Clip(float value, vtkDataArray *cellScalars, 
                            vtkPointLocator *locator, vtkCellArray *tris,
                            vtkPointData *inPd, vtkPointData *outPd,
                            vtkCellData *inCd, vtkIdType cellId,
                            vtkCellData *outCd, int insideOut)
{
  int i, numTris=this->Points->GetNumberOfPoints()-2;
  int id1, id2, id3;
  vtkDataArray *triScalars=cellScalars->MakeObject();
  triScalars->SetNumberOfTuples(3);

  for ( i=0; i < numTris; i++)
    {
    if (i % 2)
      {
      id1 = i + 2; id2 = i + 1; id3 = i;
      }
    else
      {
      id1 = i; id2 = i + 1; id3 = i + 2;
      }
  
    this->Triangle->Points->SetPoint(0,this->Points->GetPoint(id1));
    this->Triangle->Points->SetPoint(1,this->Points->GetPoint(id2));
    this->Triangle->Points->SetPoint(2,this->Points->GetPoint(id3));

    this->Triangle->PointIds->SetId(0,this->PointIds->GetId(id1));
    this->Triangle->PointIds->SetId(1,this->PointIds->GetId(id2));
    this->Triangle->PointIds->SetId(2,this->PointIds->GetId(id3));

    triScalars->SetTuple(0,cellScalars->GetTuple(id1));
    triScalars->SetTuple(1,cellScalars->GetTuple(id2));
    triScalars->SetTuple(2,cellScalars->GetTuple(id3));

    this->Triangle->Clip(value, triScalars, locator, tris, inPd, outPd, 
			inCd, cellId, outCd, insideOut);
    }

  triScalars->Delete();
}

// Return the center of the point cloud in parametric coordinates.
int vtkTriangleStrip::GetParametricCenter(float pcoords[3])
{
  pcoords[0] = pcoords[1] = 0.333333; pcoords[2] = 0.0;
  return ((this->Points->GetNumberOfPoints()-2) / 2);
}
