/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyLine.cxx
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
#include "vtkPolyLine.h"
#include "vtkMath.h"
#include "vtkCellArray.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkPolyLine* vtkPolyLine::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPolyLine");
  if(ret)
    {
    return (vtkPolyLine*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPolyLine;
}




vtkPolyLine::vtkPolyLine()
{
  this->Line = vtkLine::New();
}

vtkPolyLine::~vtkPolyLine()
{
  this->Line->Delete();
}

vtkCell *vtkPolyLine::MakeObject()
{
  vtkCell *cell = vtkPolyLine::New();
  cell->DeepCopy(this);
  return cell;
}

// Given points and lines, compute normals to lines. These are not true 
// normals, they are "orientation" normals used by classes like vtkTubeFilter
// that control the rotation around the line. The normals try to stay pointing
// in the same direction as much as possible (i.e., minimal rotation).
int vtkPolyLine::GenerateSlidingNormals(vtkPoints *pts, vtkCellArray *lines, vtkNormals *normals)
{
  vtkIdType npts;
  vtkIdType *linePts;
  float sPrev[3], sNext[3], q[3], w[3], normal[3], theta;
  float p[3], pNext[3];
  float c[3], f1, f2;
  int i, j, largeRotation;

  //  Loop over all lines
  // 
  for (lines->InitTraversal(); lines->GetNextCell(npts,linePts); )
    {
    //  Determine initial starting normal
    // 
    if ( npts <= 0 )
      {
      continue;
      }

    else if ( npts == 1 ) //return arbitrary
      {
      normal[0] = normal[1] = 0.0;
      normal[2] = 1.0;
      normals->InsertNormal(linePts[0],normal);
      }

    else //more than one point
      {
      //  Compute first normal. All "new" normals try to point in the same 
      //  direction.
      //
      for (j=0; j<npts; j++) 
        {

        if ( j == 0 ) //first point
          {
          pts->GetPoint(linePts[0],p);
          pts->GetPoint(linePts[1],pNext);

          for (i=0; i<3; i++) 
            {
            sPrev[i] = pNext[i] - p[i];
            sNext[i] = sPrev[i];
            }

          if ( vtkMath::Normalize(sNext) == 0.0 )
            {
            vtkErrorMacro(<<"Coincident points in polyline...can't compute normals");
            return 0;
            }

	  // the following logic will produce a normal orthogonal
	  // to the first line segment. If we have three points
	  // we use special logic to select a normal orthogonal
	  // too the first two line segments
	  if (npts > 2)
	    {
	    float ftmp[3];
	    
	    pts->GetPoint(linePts[2],ftmp);
            for (i=0; i<3; i++) 
              {
              ftmp[i] = ftmp[i] - pNext[i];
              }
            if ( vtkMath::Normalize(ftmp) == 0.0 )
              {
              vtkErrorMacro(<<"Coincident points in polyline...can't compute normals");
              return 0;
              }
	    // now the starting normal should simply be the cross product
	    // in the following if statement we check for the case where
	    /// the first three points are colinear 
            vtkMath::Cross(sNext,ftmp,normal);
	    }
          if ((npts <= 2)|| (vtkMath::Normalize(normal) == 0.0)) 
	    {
	    for (i=0; i<3; i++) 
	      {
	      // a little trick to find othogonal normal
	      if ( sNext[i] != 0.0 ) 
		{
		normal[(i+2)%3] = 0.0;
		normal[(i+1)%3] = 1.0;
		normal[i] = -sNext[(i+1)%3]/sNext[i];
		break;
		}
	      }
	    }
          vtkMath::Normalize(normal);
          normals->InsertNormal(linePts[0],normal);
          }

        else if ( j == (npts-1) ) //last point; just insert previous
          {
          normals->InsertNormal(linePts[j],normal);
          }

        else //inbetween points
          {
          //  Generate normals for new point by projecting previous normal
          for (i=0; i<3; i++)
            {
            p[i] = pNext[i];
            }
          pts->GetPoint(linePts[j+1],pNext);

          for (i=0; i<3; i++) 
            {
            sPrev[i] = sNext[i];
            sNext[i] = pNext[i] - p[i];
            }

          if ( vtkMath::Normalize(sNext) == 0.0 )
            {
            vtkErrorMacro(<<"Coincident points in polyline...can't compute normals");
            return 0;
            }

          //compute rotation vector
          vtkMath::Cross(sPrev,normal,w);
          if ( vtkMath::Normalize(w) == 0.0 ) 
            {
            vtkErrorMacro(<<"normal and sPrev coincident");
            return 0;
            }

          //see whether we rotate greater than 90 degrees.
          if ( vtkMath::Dot(sPrev,sNext) < 0.0 )
            {
            largeRotation = 1;
            }
          else
            {
            largeRotation = 0;
            }

          //compute rotation of line segment
          vtkMath::Cross (sNext, sPrev, q);
          if ( (theta=asin((double)vtkMath::Normalize(q))) == 0.0 ) 
            { //no rotation, use previous normal
            normals->InsertNormal(linePts[j],normal);
            continue;
            }
          if ( largeRotation )
            {
            if ( theta > 0.0 )
              {
              theta = vtkMath::Pi() - theta;
              }
            else
              {
              theta = -vtkMath::Pi() - theta;
              }
            }

          // new method
          for (i=0; i<3; i++)
            {
            c[i] = sNext[i] + sPrev[i];
            }
          vtkMath::Normalize(c);
          f1 = vtkMath::Dot(q,normal);
          f2 = 1.0 - f1*f1;
          if (f2 > 0.0)
            {
            f2 = sqrt(1.0 - f1*f1);
            }
          else
            {
            f2 = 0.0;
            }
          vtkMath::Cross(c,q,w);
          vtkMath::Cross(sPrev,q,c);
          if (vtkMath::Dot(normal,c)*vtkMath::Dot(w,c) < 0)
            {
            f2 = -1.0*f2;
            }
          for (i=0; i<3; i++)
            {
            normal[i] = f1*q[i] + f2*w[i];
            }
          
          normals->InsertNormal(linePts[j],normal);
          }//for this point
        }//else
      }//else if
    }//for this line
  return 1;
}

int vtkPolyLine::EvaluatePosition(float x[3], float* closestPoint,
                                 int& subId, float pcoords[3], 
                                 float& minDist2, float *weights)
{
  float closest[3];
  float pc[3], dist2;
  int ignoreId, i, return_status, status;
  float lineWeights[2];

  pcoords[1] = pcoords[2] = 0.0;

  return_status = 0;
  weights[0] = 0.0;
  for (minDist2=VTK_LARGE_FLOAT,i=0; i<this->Points->GetNumberOfPoints()-1; i++)
    {
    this->Line->Points->SetPoint(0,this->Points->GetPoint(i));
    this->Line->Points->SetPoint(1,this->Points->GetPoint(i+1));
    status = this->Line->EvaluatePosition(x,closest,ignoreId,pc,
                                          dist2,lineWeights);
    if ( status != -1 && dist2 < minDist2 )
      {
      return_status = status;
      if (closestPoint)
	{
	closestPoint[0] = closest[0]; 
	closestPoint[1] = closest[1]; 
	closestPoint[2] = closest[2]; 
	}
      minDist2 = dist2;
      subId = i;
      pcoords[0] = pc[0];
      weights[i] = lineWeights[0];
      weights[i+1] = lineWeights[1];
      }
    else
      {
      weights[i+1] = 0.0;
      }
    }

  return return_status;
}

void vtkPolyLine::EvaluateLocation(int& subId, float pcoords[3], float x[3],
                                   float *weights)
{
  int i;
  float *a1 = this->Points->GetPoint(subId);
  float *a2 = this->Points->GetPoint(subId+1);

  for (i=0; i<3; i++) 
    {
    x[i] = a1[i] + pcoords[0]*(a2[i] - a1[i]);
    }
  
  weights[0] = 1.0 - pcoords[0];
  weights[1] = pcoords[0];
}

int vtkPolyLine::CellBoundary(int subId, float pcoords[3], vtkIdList *pts)
{
  pts->SetNumberOfIds(1);

  if ( pcoords[0] >= 0.5 )
    {
    pts->SetId(0,this->PointIds->GetId(subId+1));
    if ( pcoords[0] > 1.0 )
      {
      return 0;
      }
    else
      {
      return 1;
      }
    }
  else
    {
    pts->SetId(0,this->PointIds->GetId(subId));
    if ( pcoords[0] < 0.0 )
      {
      return 0;
      }
    else
      {
      return 1;
      }
    }
}

void vtkPolyLine::Contour(float value, vtkScalars *cellScalars,
                         vtkPointLocator *locator, vtkCellArray *verts, 
                         vtkCellArray *lines, vtkCellArray *polys, 
                         vtkPointData *inPd, vtkPointData *outPd,
                         vtkCellData *inCd, int cellId, vtkCellData *outCd)
{
  int i, numLines=this->Points->GetNumberOfPoints() - 1;
  vtkScalars *lineScalars=vtkScalars::New();
  lineScalars->SetNumberOfScalars(2);

  for ( i=0; i < numLines; i++)
    {
    this->Line->Points->SetPoint(0,this->Points->GetPoint(i));
    this->Line->Points->SetPoint(1,this->Points->GetPoint(i+1));

    if ( outPd )
      {
      this->Line->PointIds->SetId(0,this->PointIds->GetId(i));
      this->Line->PointIds->SetId(1,this->PointIds->GetId(i+1));
      }

    lineScalars->SetScalar(0,cellScalars->GetScalar(i));
    lineScalars->SetScalar(1,cellScalars->GetScalar(i+1));

    this->Line->Contour(value, lineScalars, locator, verts,
                       lines, polys, inPd, outPd, inCd, cellId, outCd);
    }
  lineScalars->Delete();
}

// Intersect with sub-lines
//
int vtkPolyLine::IntersectWithLine(float p1[3], float p2[3],float tol,float& t,
                                  float x[3], float pcoords[3], int& subId)
{
  int subTest, numLines=this->Points->GetNumberOfPoints() - 1;

  for (subId=0; subId < numLines; subId++)
    {
    this->Line->Points->SetPoint(0,this->Points->GetPoint(subId));
    this->Line->Points->SetPoint(1,this->Points->GetPoint(subId+1));

    if ( this->Line->IntersectWithLine(p1, p2, tol, t, x, pcoords, subTest) )
      {
      return 1;
      }
    }

  return 0;
}

int vtkPolyLine::Triangulate(int vtkNotUsed(index), vtkIdList *ptIds,
                             vtkPoints *pts)
{
  int numLines=this->Points->GetNumberOfPoints() - 1;
  pts->Reset();
  ptIds->Reset();

  for (int subId=0; subId < numLines; subId++)
    {
    pts->InsertNextPoint(this->Points->GetPoint(subId));
    ptIds->InsertNextId(this->PointIds->GetId(subId));

    pts->InsertNextPoint(this->Points->GetPoint(subId+1));
    ptIds->InsertNextId(this->PointIds->GetId(subId+1));
    }

  return 1;
}

void vtkPolyLine::Derivatives(int subId, float pcoords[3], float *values, 
                              int dim, float *derivs)
{
  this->Line->PointIds->SetNumberOfIds(2);

  this->Line->Points->SetPoint(0,this->Points->GetPoint(subId));
  this->Line->Points->SetPoint(1,this->Points->GetPoint(subId+1));

  this->Line->Derivatives(0, pcoords, values+dim*subId, dim, derivs);
}

void vtkPolyLine::Clip(float value, vtkScalars *cellScalars, 
                       vtkPointLocator *locator, vtkCellArray *lines,
                       vtkPointData *inPd, vtkPointData *outPd,
                       vtkCellData *inCd, int cellId, vtkCellData *outCd,
                       int insideOut)
{
  int i, numLines=this->Points->GetNumberOfPoints() - 1;
  vtkScalars *lineScalars=vtkScalars::New();
  lineScalars->SetNumberOfScalars(2);

  for ( i=0; i < numLines; i++)
    {
    this->Line->Points->SetPoint(0,this->Points->GetPoint(i));
    this->Line->Points->SetPoint(1,this->Points->GetPoint(i+1));

    this->Line->PointIds->SetId(0,this->PointIds->GetId(i));
    this->Line->PointIds->SetId(1,this->PointIds->GetId(i+1));

    lineScalars->SetScalar(0,cellScalars->GetScalar(i));
    lineScalars->SetScalar(1,cellScalars->GetScalar(i+1));

    this->Line->Clip(value, lineScalars, locator, lines, inPd, outPd, 
                    inCd, cellId, outCd, insideOut);
    }
  
  lineScalars->Delete();
}

// Return the center of the point cloud in parametric coordinates.
int vtkPolyLine::GetParametricCenter(float pcoords[3])
{
  pcoords[0] = 0.5; pcoords[1] = pcoords[2] = 0.0;
  return ((this->Points->GetNumberOfPoints() - 1) / 2);
}
