/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVoxel.cxx
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
#include "vtkVoxel.h"
#include "vtkMath.h"
#include "vtkLine.h"
#include "vtkPixel.h"
#include "vtkCellArray.h"
#include "vtkPointLocator.h"

// Construct the voxel with eight points.
vtkVoxel::vtkVoxel()
{
  int i;
  
  this->Points->SetNumberOfPoints(8);
  this->PointIds->SetNumberOfIds(8);
  for (i = 0; i < 8; i++)
    {
    this->Points->SetPoint(i, 0.0, 0.0, 0.0);
    }
  for (i = 0; i < 8; i++)
    {
    this->PointIds->SetId(i,0);
    }
  this->Line = vtkLine::New();
  this->Pixel = vtkPixel::New();
}

vtkVoxel::~vtkVoxel()
{
  this->Line->Delete();
  this->Pixel->Delete();
}

vtkCell *vtkVoxel::MakeObject()
{
  vtkCell *cell = vtkVoxel::New();
  cell->DeepCopy(this);
  return cell;
}

int vtkVoxel::EvaluatePosition(float x[3], float* closestPoint,
                              int& subId, float pcoords[3], 
                              float& dist2, float *weights)
{
  float *pt1, *pt2, *pt3, *pt4;
  int i;

  subId = 0;
//
// Get coordinate system
//
  pt1 = this->Points->GetPoint(0);
  pt2 = this->Points->GetPoint(1);
  pt3 = this->Points->GetPoint(2);
  pt4 = this->Points->GetPoint(4);
//
// Develop parametric coordinates
//
  pcoords[0] = (x[0] - pt1[0]) / (pt2[0] - pt1[0]);
  pcoords[1] = (x[1] - pt1[1]) / (pt3[1] - pt1[1]);
  pcoords[2] = (x[2] - pt1[2]) / (pt4[2] - pt1[2]);

  if ( pcoords[0] >= 0.0 && pcoords[0] <= 1.0 &&
  pcoords[1] >= 0.0 && pcoords[1] <= 1.0 &&
  pcoords[2] >= 0.0 && pcoords[2] <= 1.0 )
    {
    if (closestPoint)
      {
      closestPoint[0] = x[0]; closestPoint[1] = x[1]; closestPoint[2] = x[2];
      }
    dist2 = 0.0; // inside voxel
    this->InterpolationFunctions(pcoords,weights);
    return 1;
    }
  else
    {
    float pc[3], w[8];
    if (closestPoint)
      {
      for (i=0; i<3; i++)
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

void vtkVoxel::EvaluateLocation(int& vtkNotUsed(subId), float pcoords[3], 
				float x[3], float *weights)
{
  float *pt1, *pt2, *pt3, *pt4;
  int i;

  pt1 = this->Points->GetPoint(0);
  pt2 = this->Points->GetPoint(1);
  pt3 = this->Points->GetPoint(2);
  pt4 = this->Points->GetPoint(4);

  for (i=0; i<3; i++)
    {
    x[i] = pt1[i] + pcoords[0]*(pt2[i] - pt1[i]) +
                    pcoords[1]*(pt3[i] - pt1[i]) +
                    pcoords[2]*(pt4[i] - pt1[i]);
    }
  
  this->InterpolationFunctions(pcoords,weights);
}

//
// Compute Interpolation functions
//
void vtkVoxel::InterpolationFunctions(float pcoords[3], float sf[8])
{
  float rm, sm, tm;

  float r = pcoords[0], s = pcoords[1], t = pcoords[2];

  rm = 1. - r;
  sm = 1. - s;
  tm = 1. - t;

  sf[0] = rm * sm * tm;
  sf[1] = r * sm * tm;
  sf[2] = rm * s * tm;
  sf[3] = r * s * tm;
  sf[4] = rm * sm * t;
  sf[5] = r * sm * t;
  sf[6] = rm * s * t;
  sf[7] = r * s * t;
}

void vtkVoxel::InterpolationDerivs(float pcoords[3], float derivs[24])
{
  double rm, sm, tm;

  rm = 1. - pcoords[0];
  sm = 1. - pcoords[1];
  tm = 1. - pcoords[2];

  // r derivatives
  derivs[0] = -sm*tm;
  derivs[1] = sm*tm;
  derivs[2] = -pcoords[1]*tm;
  derivs[3] = pcoords[1]*tm;
  derivs[4] = -sm*pcoords[2];
  derivs[5] = sm*pcoords[2];
  derivs[6] = -pcoords[1]*pcoords[2];
  derivs[7] = pcoords[1]*pcoords[2];

  // s derivatives
  derivs[8] = -rm*tm;
  derivs[9] = -pcoords[0]*tm;
  derivs[10] = rm*tm;
  derivs[11] = pcoords[0]*tm;
  derivs[12] = -rm*pcoords[2];
  derivs[13] = -pcoords[0]*pcoords[2];
  derivs[14] = rm*pcoords[2];
  derivs[15] = pcoords[0]*pcoords[2];

  // t derivatives
  derivs[16] = -rm*sm;
  derivs[17] = -pcoords[0]*sm;
  derivs[18] = -rm*pcoords[1];
  derivs[19] = -pcoords[0]*pcoords[1];
  derivs[20] = rm*sm;
  derivs[21] = pcoords[0]*sm;
  derivs[22] = rm*pcoords[1];
  derivs[23] = pcoords[0]*pcoords[1];
}

int vtkVoxel::CellBoundary(int vtkNotUsed(subId), float pcoords[3],
			   vtkIdList *pts)
{
  float t1=pcoords[0]-pcoords[1];
  float t2=1.0-pcoords[0]-pcoords[1];
  float t3=pcoords[1]-pcoords[2];
  float t4=1.0-pcoords[1]-pcoords[2];
  float t5=pcoords[2]-pcoords[0];
  float t6=1.0-pcoords[2]-pcoords[0];

  pts->SetNumberOfIds(4);

  // compare against six planes in parametric space that divide element
  // into six pieces.
  if ( t3 >= 0.0 && t4 >= 0.0 && t5 < 0.0 && t6 >= 0.0 )
    {
    pts->SetId(0,this->PointIds->GetId(0));
    pts->SetId(1,this->PointIds->GetId(1));
    pts->SetId(2,this->PointIds->GetId(3));
    pts->SetId(3,this->PointIds->GetId(2));
    }

  else if ( t1 >= 0.0 && t2 < 0.0 && t5 < 0.0 && t6 < 0.0 )
    {
    pts->SetId(0,this->PointIds->GetId(1));
    pts->SetId(1,this->PointIds->GetId(3));
    pts->SetId(2,this->PointIds->GetId(7));
    pts->SetId(3,this->PointIds->GetId(5));
    }

  else if ( t1 >= 0.0 && t2 >= 0.0 && t3 < 0.0 && t4 >= 0.0 )
    {
    pts->SetId(0,this->PointIds->GetId(0));
    pts->SetId(1,this->PointIds->GetId(1));
    pts->SetId(2,this->PointIds->GetId(5));
    pts->SetId(3,this->PointIds->GetId(4));
    }

  else if ( t3 < 0.0 && t4 < 0.0 && t5 >= 0.0 && t6 < 0.0 )
    {
    pts->SetId(0,this->PointIds->GetId(4));
    pts->SetId(1,this->PointIds->GetId(5));
    pts->SetId(2,this->PointIds->GetId(7));
    pts->SetId(3,this->PointIds->GetId(6));
    }

  else if ( t1 < 0.0 && t2 >= 0.0 && t5 >= 0.0 && t6 >= 0.0 )
    {
    pts->SetId(0,this->PointIds->GetId(0));
    pts->SetId(1,this->PointIds->GetId(4));
    pts->SetId(2,this->PointIds->GetId(6));
    pts->SetId(3,this->PointIds->GetId(2));
    }

  else // if ( t1 < 0.0 && t2 < 0.0 && t3 >= 0.0 && t6 < 0.0 )
    {
    pts->SetId(0,this->PointIds->GetId(3));
    pts->SetId(1,this->PointIds->GetId(2));
    pts->SetId(2,this->PointIds->GetId(6));
    pts->SetId(3,this->PointIds->GetId(7));
    }

  if ( pcoords[0] < 0.0 || pcoords[0] > 1.0 ||
       pcoords[1] < 0.0 || pcoords[1] > 1.0 ||
       pcoords[2] < 0.0 || pcoords[2] > 1.0 )
    {
    return 0;
    }
  else
    {
    return 1;
    }
}

static int edges[12][2] = { {0,1}, {1,3}, {2,3}, {0,2},
                            {4,5}, {5,7}, {6,7}, {4,6},
                            {0,4}, {1,5}, {2,6}, {3,7}};
// define in terms vtkPixel understands
static int faces[6][4] = { {2,0,6,4}, {1,3,5,7},
                           {0,1,4,5}, {3,2,7,6},
                           {1,0,3,2}, {4,5,6,7} };

//
// Marching cubes case table
//
#include "vtkMarchingCubesCases.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkVoxel* vtkVoxel::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkVoxel");
  if(ret)
    {
    return (vtkVoxel*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkVoxel;
}




void vtkVoxel::Contour(float value, vtkDataArray *cellScalars, 
		       vtkPointLocator *locator,
		       vtkCellArray *vtkNotUsed(verts), 
		       vtkCellArray *vtkNotUsed(lines), 
		       vtkCellArray *polys,
                       vtkPointData *inPd, vtkPointData *outPd,
                       vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd)
{
  static int CASE_MASK[8] = {1,2,4,8,16,32,64,128};
  VTK_TRIANGLE_CASES *triCase;
  EDGE_LIST  *edge;
  int i, j, index, *vert;
  static int vertMap[8] = { 0, 1, 3, 2, 4, 5, 7, 6 };
  int newCellId;
  vtkIdType pts[3];
  float t, *x1, *x2, x[3];

  // Build the case table
  for ( i=0, index = 0; i < 8; i++)
    {
    if (cellScalars->GetComponent(vertMap[i],0) >= value)
      {
      index |= CASE_MASK[i];
      }
    }
  
  triCase = VTK_TRIANGLE_CASES::GetCases() + index;
  edge = triCase->edges;

  for ( ; edge[0] > -1; edge += 3 )
    {
    for (i=0; i<3; i++) // insert triangle
      {
      vert = edges[edge[i]];
      t = (value - cellScalars->GetComponent(vert[0],0)) /
          (cellScalars->GetComponent(vert[1],0) 
	   - cellScalars->GetComponent(vert[0],0));
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
    // check for degenerate triangle
    if ( pts[0] != pts[1] &&
	 pts[0] != pts[2] &&
	 pts[1] != pts[2] )
      {
      newCellId = polys->InsertNextCell(3,pts);
      outCd->CopyData(inCd,cellId,newCellId);
      }
    }
}


int *vtkVoxel::GetEdgeArray(int edgeId)
{
  return edges[edgeId];
}
vtkCell *vtkVoxel::GetEdge(int edgeId)
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

int *vtkVoxel::GetFaceArray(int faceId)
{
  return faces[faceId];
}
vtkCell *vtkVoxel::GetFace(int faceId)
{
  int *verts, i;

  verts = faces[faceId];

  for (i=0; i<4; i++)
    {
    this->Pixel->PointIds->SetId(i,this->PointIds->GetId(verts[i]));
    this->Pixel->Points->SetPoint(i,this->Points->GetPoint(verts[i]));
    }

  return this->Pixel;
}

// 
// Intersect voxel with line using "bounding box" intersection.
//
int vtkVoxel::IntersectWithLine(float p1[3], float p2[3], float vtkNotUsed(tol), 
                               float& t, float x[3], float pcoords[3], int& subId)
{
  float *minPt, *maxPt;
  float bounds[6], p21[3];
  int i;

  subId = 0;

  minPt = this->Points->GetPoint(0);
  maxPt = this->Points->GetPoint(7);

  for (i=0; i<3; i++)
    {
    p21[i] = p2[i] - p1[i];
    bounds[2*i] = minPt[i];
    bounds[2*i+1] = maxPt[i];
    }

  if ( ! vtkCell::HitBBox(bounds, p1, p21, x, t) )
    {
    return 0;
    }
  //
  // Evaluate intersection
  //
  for (i=0; i<3; i++)
    {
    pcoords[i] = (x[i] - minPt[i]) / (maxPt[i] - minPt[i]);
    }

  return 1;
}

int vtkVoxel::Triangulate(int index, vtkIdList *ptIds, vtkPoints *pts)
{
  int p[4], i;

  ptIds->Reset();
  pts->Reset();
  //
  // Create five tetrahedron. Triangulation varies depending upon index. This
  // is necessary to insure compatible voxel triangulations.
  //
  if ( (index % 2) )
    {
    p[0] = 0; p[1] = 1; p[2] = 4; p[3] = 2;
    for ( i=0; i < 4; i++ )
      {
      ptIds->InsertNextId(this->PointIds->GetId(p[i]));
      pts->InsertNextPoint(this->Points->GetPoint(p[i]));
      }

    p[0] = 1; p[1] = 4; p[2] = 7; p[3] = 5;
    for ( i=0; i < 4; i++ )
      {
      ptIds->InsertNextId(this->PointIds->GetId(p[i]));
      pts->InsertNextPoint(this->Points->GetPoint(p[i]));
      }

    p[0] = 1; p[1] = 4; p[2] = 2; p[3] = 7;
    for ( i=0; i < 4; i++ )
      {
      ptIds->InsertNextId(this->PointIds->GetId(p[i]));
      pts->InsertNextPoint(this->Points->GetPoint(p[i]));
      }

    p[0] = 1; p[1] = 2; p[2] = 3; p[3] = 7;
    for ( i=0; i < 4; i++ )
      {
      ptIds->InsertNextId(this->PointIds->GetId(p[i]));
      pts->InsertNextPoint(this->Points->GetPoint(p[i]));
      }

    p[0] = 2; p[1] = 7; p[2] = 4; p[3] = 6;
    for ( i=0; i < 4; i++ )
      {
      ptIds->InsertNextId(this->PointIds->GetId(p[i]));
      pts->InsertNextPoint(this->Points->GetPoint(p[i]));
      }
    }
  else
    {
    p[0] = 3; p[1] = 1; p[2] = 0; p[3] = 5;
    for ( i=0; i < 4; i++ )
      {
      ptIds->InsertNextId(this->PointIds->GetId(p[i]));
      pts->InsertNextPoint(this->Points->GetPoint(p[i]));
      }

    p[0] = 0; p[1] = 3; p[2] = 6; p[3] = 2;
    for ( i=0; i < 4; i++ )
      {
      ptIds->InsertNextId(this->PointIds->GetId(p[i]));
      pts->InsertNextPoint(this->Points->GetPoint(p[i]));
      }

    p[0] = 3; p[1] = 5; p[2] = 6; p[3] = 7;
    for ( i=0; i < 4; i++ )
      {
      ptIds->InsertNextId(this->PointIds->GetId(p[i]));
      pts->InsertNextPoint(this->Points->GetPoint(p[i]));
      }

    p[0] = 0; p[1] = 6; p[2] = 5; p[3] = 4;
    for ( i=0; i < 4; i++ )
      {
      ptIds->InsertNextId(this->PointIds->GetId(p[i]));
      pts->InsertNextPoint(this->Points->GetPoint(p[i]));
      }

    p[0] = 0; p[1] = 3; p[2] = 5; p[3] = 6;
    for ( i=0; i < 4; i++ )
      {
      ptIds->InsertNextId(this->PointIds->GetId(p[i]));
      pts->InsertNextPoint(this->Points->GetPoint(p[i]));
      }
    }

  return 1;
}

void vtkVoxel::Derivatives(int vtkNotUsed(subId), float pcoords[3], 
                           float *values, int dim, float *derivs)
{
  float functionDerivs[24], sum;
  int i, j, k;
  float *x0, *x1, *x2, *x4, spacing[3];

  x0 = this->Points->GetPoint(0);
  x1 = this->Points->GetPoint(1);
  spacing[0] = x1[0] - x0[0];

  x2 = this->Points->GetPoint(2);
  spacing[1] = x2[1] - x0[1];

  x4 = this->Points->GetPoint(4);
  spacing[2] = x4[2] - x0[2];

  // get derivatives in r-s-t directions
  this->InterpolationDerivs(pcoords, functionDerivs);

  // since the x-y-z axes are aligned with r-s-t axes, only need to scale
  // the derivative values by the data spacing.
  for (k=0; k < dim; k++) //loop over values per vertex
    {
    for (j=0; j < 3; j++) //loop over derivative directions
      {
      for (sum=0.0, i=0; i < 8; i++) //loop over interp. function derivatives
        {
        sum += functionDerivs[8*j + i] * values[dim*i + k];
        }
      derivs[3*k + j] = sum / spacing[j];
      }
    }
}

void vtkVoxel::GetEdgePoints(int edgeId, int* &pts)
{
  pts = this->GetEdgeArray(edgeId);
}

void vtkVoxel::GetFacePoints(int faceId, int* &pts)
{
  pts = this->GetFaceArray(faceId);
}
