/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTriangleStrip.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkTriangleStrip.h"
#include "vtkCellArray.h"
#include "vtkLine.h"

vtkCell *vtkTriangleStrip::MakeObject()
{
  vtkCell *cell = vtkTriangleStrip::New();
  cell->DeepCopy(*this);
  return cell;
}

int vtkTriangleStrip::EvaluatePosition(float x[3], float closestPoint[3],
                                      int& subId, float pcoords[3], 
                                      float& minDist2, float *weights)
{
  float pc[3], dist2;
  int ignoreId, i, return_status, status;
  float tempWeights[3], activeWeights[3];
  float closest[3];

  pcoords[2] = 0.0;

  return_status = 0;
  for (minDist2=VTK_LARGE_FLOAT,i=0; i<this->Points.GetNumberOfPoints()-2; i++)
    {
    weights[i] = 0.0;
    this->Triangle.Points.SetPoint(0,this->Points.GetPoint(i));
    this->Triangle.Points.SetPoint(1,this->Points.GetPoint(i+1));
    this->Triangle.Points.SetPoint(2,this->Points.GetPoint(i+2));
    status = this->Triangle.EvaluatePosition(x,closest,ignoreId,pc,dist2,tempWeights);
    if ( status != -1 && dist2 < minDist2 )
      {
      return_status = status;
      closestPoint[0] = closest[0]; closestPoint[1] = closest[1]; closestPoint[2] = closest[2];
      subId = i;
      pcoords[0] = pc[0];
      pcoords[1] = pc[1];
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
  float *pt1 = this->Points.GetPoint(subId);
  float *pt2 = this->Points.GetPoint(subId+1);
  float *pt3 = this->Points.GetPoint(subId+2);
  float u3 = 1.0 - pcoords[0] - pcoords[1];

  for (i=0; i<3; i++)
    {
    x[i] = pt1[i]*pcoords[0] + pt2[i]*pcoords[1] + pt3[i]*u3;
    }

  weights[0] = u3;
  weights[1] = pcoords[0];
  weights[2] = pcoords[1];
}

int vtkTriangleStrip::CellBoundary(int subId, float pcoords[3], vtkIdList& pts)
{
  int numPts=this->Points.GetNumberOfPoints();
  int retStatus;

  this->Triangle.Points.SetPoint(0,this->Points.GetPoint(subId));
  this->Triangle.Points.SetPoint(1,this->Points.GetPoint(subId+1));
  this->Triangle.Points.SetPoint(2,this->Points.GetPoint(subId+2));
  retStatus = this->Triangle.CellBoundary(0, pcoords, pts);

  if ( subId > 0 && subId < (numPts-3) ) //in the middle of the strip
    {
    pts.InsertId(0,this->PointIds.GetId(subId));
    pts.InsertId(1,this->PointIds.GetId(subId+2));
    }
  else if ( subId <= 0 ) //first triangle
    {
    pts.InsertId(0,this->PointIds.GetId(0));
    if ( pcoords[1] > pcoords[0] )
      {
      pts.InsertId(1,this->PointIds.GetId(1));
      }
    else
      {
      pts.InsertId(1,this->PointIds.GetId(2));
      }
    }
  else //last triangle
    {
    pts.InsertId(0,this->PointIds.GetId(numPts-1));
    if ( pcoords[0] < (1.0 - pcoords[0] - pcoords[1]) )
      {
      pts.InsertId(1,this->PointIds.GetId(numPts-3));
      }
    else
      {
      pts.InsertId(1,this->PointIds.GetId(numPts-2));
      }
    }
  return retStatus;

}

void vtkTriangleStrip::Contour(float value, vtkScalars *cellScalars, 
                              vtkPointLocator *locator, vtkCellArray *verts, 
                              vtkCellArray *lines, vtkCellArray *polys, 
                              vtkPointData *inPd, vtkPointData *outPd,
                              vtkCellData *inCd, int cellId, vtkCellData *outCd)
{
  int i, numTris=this->Points.GetNumberOfPoints()-2;
  vtkScalars *triScalars=vtkScalars::New();
  triScalars->SetNumberOfScalars(3);

  for ( i=0; i < numTris; i++)
    {
    this->Triangle.Points.SetPoint(0,this->Points.GetPoint(i));
    this->Triangle.Points.SetPoint(1,this->Points.GetPoint(i+1));
    this->Triangle.Points.SetPoint(2,this->Points.GetPoint(i+2));

    if ( outPd )
      {
      this->Triangle.PointIds.SetId(0,this->PointIds.GetId(i));
      this->Triangle.PointIds.SetId(1,this->PointIds.GetId(i+1));
      this->Triangle.PointIds.SetId(2,this->PointIds.GetId(i+2));
      }

    triScalars->SetScalar(0,cellScalars->GetScalar(i));
    triScalars->SetScalar(1,cellScalars->GetScalar(i+1));
    triScalars->SetScalar(2,cellScalars->GetScalar(i+2));

    this->Triangle.Contour(value, triScalars, locator, verts,
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

  this->Line.PointIds.SetId(0,this->PointIds.GetId(id1));
  this->Line.PointIds.SetId(1,this->PointIds.GetId(id2));
  this->Line.Points.SetPoint(0,this->Points.GetPoint(id1));
  this->Line.Points.SetPoint(1,this->Points.GetPoint(id2));

  return &this->Line;
}

// 
// Intersect sub-triangles
//
int vtkTriangleStrip::IntersectWithLine(float p1[3], float p2[3], float tol,
                                       float& t, float x[3], float pcoords[3],
                                       int& subId)
{
  int subTest, numTris=this->Points.GetNumberOfPoints()-2;

  for (subId=0; subId < numTris; subId++)
    {
    this->Triangle.Points.SetPoint(0,this->Points.GetPoint(subId));
    this->Triangle.Points.SetPoint(1,this->Points.GetPoint(subId+1));
    this->Triangle.Points.SetPoint(2,this->Points.GetPoint(subId+2));

    if ( this->Triangle.IntersectWithLine(p1, p2, tol, t, x, pcoords, subTest) )
      return 1;
    }

  return 0;
}

int vtkTriangleStrip::Triangulate(int vtkNotUsed(index), vtkIdList &ptIds, 
                                  vtkPoints &pts)
{
  int numTris=this->Points.GetNumberOfPoints()-2;
  pts.Reset();
  ptIds.Reset();

  for (int subId=0; subId < numTris; subId++)
    {
    for ( int i=0; i < 3; i++ )
      {
      ptIds.InsertNextId(this->PointIds.GetId(subId+i));
      pts.InsertNextPoint(this->Points.GetPoint(subId+i));
      }
    }

  return 1;
}

void vtkTriangleStrip::Derivatives(int subId, float pcoords[3], float *values, 
                                   int dim, float *derivs)
{

  this->Triangle.Points.SetPoint(0,this->Points.GetPoint(subId));
  this->Triangle.Points.SetPoint(1,this->Points.GetPoint(subId+1));
  this->Triangle.Points.SetPoint(2,this->Points.GetPoint(subId+2));

  this->Triangle.Derivatives(0, pcoords, values, dim, derivs);
}

// Description:
// Given a list of triangle strips, decompose into a list of (triangle) 
// polygons. The polygons are appended to the end of the list of polygons.
void vtkTriangleStrip::DecomposeStrips(vtkCellArray *strips, vtkCellArray *polys)
{
  int npts, *pts, p1, p2, p3, i;

  for (strips->InitTraversal(); strips->GetNextCell(npts,pts); )
    {
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
}

void vtkTriangleStrip::Clip(float value, vtkScalars *cellScalars, 
                            vtkPointLocator *locator, vtkCellArray *tris,
                            vtkPointData *inPd, vtkPointData *outPd,
                            vtkCellData *inCd, int cellId, vtkCellData *outCd,
                            int insideOut)
{
  int i, numTris=this->Points.GetNumberOfPoints()-2;
  int id1, id2, id3;
  vtkScalars *triScalars=vtkScalars::New();
  triScalars->SetNumberOfScalars(3);

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
  
    this->Triangle.Points.SetPoint(0,this->Points.GetPoint(id1));
    this->Triangle.Points.SetPoint(1,this->Points.GetPoint(id2));
    this->Triangle.Points.SetPoint(2,this->Points.GetPoint(id3));

    this->Triangle.PointIds.SetId(0,this->PointIds.GetId(id1));
    this->Triangle.PointIds.SetId(1,this->PointIds.GetId(id2));
    this->Triangle.PointIds.SetId(2,this->PointIds.GetId(id3));

    triScalars->SetScalar(0,cellScalars->GetScalar(id1));
    triScalars->SetScalar(1,cellScalars->GetScalar(id2));
    triScalars->SetScalar(2,cellScalars->GetScalar(id3));

    this->Triangle.Clip(value, triScalars, locator, tris, inPd, outPd, 
			inCd, cellId, outCd, insideOut);
    }

  triScalars->Delete();
}

// Description:
// Return the center of the point cloud in parametric coordinates.
inline int vtkTriangleStrip::GetParametricCenter(float pcoords[3])
{
  pcoords[0] = pcoords[1] = 0.333333; pcoords[2] = 0.0;
  return ((this->Points.GetNumberOfPoints()-2) / 2);
}
