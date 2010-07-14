/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyLine.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPolyLine.h"

#include "vtkMath.h"
#include "vtkCellArray.h"
#include "vtkObjectFactory.h"
#include "vtkDoubleArray.h"
#include "vtkLine.h"
#include "vtkPoints.h"
#include "vtkIncrementalPointLocator.h"

#include <vtkstd/algorithm>

vtkStandardNewMacro(vtkPolyLine);

//----------------------------------------------------------------------------
vtkPolyLine::vtkPolyLine()
{
  this->Line = vtkLine::New();
}

//----------------------------------------------------------------------------
vtkPolyLine::~vtkPolyLine()
{
  this->Line->Delete();
}

//----------------------------------------------------------------------------
int vtkPolyLine::GenerateSlidingNormals(vtkPoints *pts, vtkCellArray *lines,
                                        vtkDataArray *normals)
{
  return vtkPolyLine::GenerateSlidingNormals(pts, lines, normals, 0);
}

//----------------------------------------------------------------------------
// Given points and lines, compute normals to lines. These are not true 
// normals, they are "orientation" normals used by classes like vtkTubeFilter
// that control the rotation around the line. The normals try to stay pointing
// in the same direction as much as possible (i.e., minimal rotation).
int vtkPolyLine::GenerateSlidingNormals(vtkPoints *pts, vtkCellArray *lines,
                                        vtkDataArray *normals, 
                                        double* firstNormal)
{
  vtkIdType npts=0;
  vtkIdType *linePts=0;
  double sPrev[3], sNext[3], q[3], w[3], normal[3], theta;
  double p[3], pNext[3];
  double c[3], f1, f2;
  int i, j, largeRotation;
  sNext[0]=0.0;
  sNext[1]=0.0;
  sNext[2]=0.0;

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
      normals->InsertTuple(linePts[0],normal);
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
            vtkGenericWarningMacro(<<"Coincident points in polyline...can't compute normals");
            return 0;
            }

          if (!firstNormal)
            {
            // the following logic will produce a normal orthogonal
            // to the first line segment. If we have three points
            // we use special logic to select a normal orthogonal
            // to the first two line segments
            int foundNormal=0;
            if (npts > 2)
              {
              int ipt;

              // Look at the line segments (0,1), (ipt-1, ipt)
              // until a pair which meets the following criteria
              // is found: ||(0,1)x(ipt-1,ipt)|| > 1.0E-3.
              // This is used to eliminate nearly parallel cases.
              for(ipt=2; ipt < npts; ipt++)
                {
                double ftmp[3], ftmp2[3];
            
                pts->GetPoint(linePts[ipt-1],ftmp);
                pts->GetPoint(linePts[ipt]  ,ftmp2);

                for (i=0; i<3; i++) 
                  {
                  ftmp[i] = ftmp2[i] - ftmp[i];
                  }

                if ( vtkMath::Normalize(ftmp) == 0.0 )
                  {
                  continue;
                  }

                // now the starting normal should simply be the cross product
                // in the following if statement we check for the case where
                // the two segments are parallel 
                vtkMath::Cross(sNext,ftmp,normal);
                if ( vtkMath::Norm(normal) > 1.0E-3 )
                  {
                  foundNormal = 1;
                  break;
                  }
                }
              }

            if ((npts <= 2)|| !foundNormal) 
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
            }
          else
            {
            memcpy(normal, firstNormal, 3*sizeof(double));
            }
          vtkMath::Normalize(normal);
          normals->InsertTuple(linePts[0],normal);
          }

        else if ( j == (npts-1) ) //last point; just insert previous
          {
          normals->InsertTuple(linePts[j],normal);
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
            vtkGenericWarningMacro(<<"Coincident points in polyline...can't compute normals");
            return 0;
            }

          //compute rotation vector
          vtkMath::Cross(sPrev,normal,w);
          if ( vtkMath::Normalize(w) == 0.0 ) 
            {
            vtkGenericWarningMacro(<<"normal and sPrev coincident");
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
          theta=asin(static_cast<double>(vtkMath::Normalize(q)));
          if (theta==0.0) 
            { //no rotation, use previous normal
            normals->InsertTuple(linePts[j],normal);
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
          
          normals->InsertTuple(linePts[j],normal);
          }//for this point
        }//else
      }//else if
    }//for this line
  return 1;
}

//----------------------------------------------------------------------------
int vtkPolyLine::EvaluatePosition(double x[3], double* closestPoint,
                                 int& subId, double pcoords[3], 
                                 double& minDist2, double *weights)
{
  double closest[3];
  double pc[3], dist2;
  int ignoreId, i, return_status, status;
  double lineWeights[2], closestWeights[2];

  pcoords[1] = pcoords[2] = 0.0;

  return_status = 0;
  subId = -1;
  closestWeights[0] = closestWeights[1] = 0.0;  // Shut up, compiler
  for (minDist2=VTK_DOUBLE_MAX,i=0; i<this->Points->GetNumberOfPoints()-1; i++)
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
      closestWeights[0] = lineWeights[0];
      closestWeights[1] = lineWeights[1];
      }
    }

  vtkstd::fill_n(weights, this->Points->GetNumberOfPoints(), 0.0);
  if (subId >= 0)
    {
    weights[subId] = closestWeights[0];
    weights[subId+1] = closestWeights[1];
    }

  return return_status;
}

//----------------------------------------------------------------------------
void vtkPolyLine::EvaluateLocation(int& subId, double pcoords[3], double x[3],
                                   double *weights)
{
  int i;
  double a1[3];
  double a2[3];
  this->Points->GetPoint(subId, a1);
  this->Points->GetPoint(subId+1, a2);

  for (i=0; i<3; i++) 
    {
    x[i] = a1[i] + pcoords[0]*(a2[i] - a1[i]);
    }
  
  weights[0] = 1.0 - pcoords[0];
  weights[1] = pcoords[0];
}

//----------------------------------------------------------------------------
int vtkPolyLine::CellBoundary(int subId, double pcoords[3], vtkIdList *pts)
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

//----------------------------------------------------------------------------
void vtkPolyLine::Contour(double value, vtkDataArray *cellScalars,
                          vtkIncrementalPointLocator *locator, vtkCellArray *verts,
                          vtkCellArray *lines, vtkCellArray *polys, 
                          vtkPointData *inPd, vtkPointData *outPd,
                          vtkCellData *inCd, vtkIdType cellId,
                          vtkCellData *outCd)
{
  int i, numLines=this->Points->GetNumberOfPoints() - 1;
  vtkDataArray *lineScalars=cellScalars->NewInstance();
  lineScalars->SetNumberOfComponents(cellScalars->GetNumberOfComponents());
  lineScalars->SetNumberOfTuples(2);

  for ( i=0; i < numLines; i++)
    {
    this->Line->Points->SetPoint(0,this->Points->GetPoint(i));
    this->Line->Points->SetPoint(1,this->Points->GetPoint(i+1));

    if ( outPd )
      {
      this->Line->PointIds->SetId(0,this->PointIds->GetId(i));
      this->Line->PointIds->SetId(1,this->PointIds->GetId(i+1));
      }

    lineScalars->SetTuple(0,cellScalars->GetTuple(i));
    lineScalars->SetTuple(1,cellScalars->GetTuple(i+1));

    this->Line->Contour(value, lineScalars, locator, verts,
                       lines, polys, inPd, outPd, inCd, cellId, outCd);
    }
  lineScalars->Delete();
}

//----------------------------------------------------------------------------
// Intersect with sub-lines
//
int vtkPolyLine::IntersectWithLine(double p1[3], double p2[3],double tol,double& t,
                                  double x[3], double pcoords[3], int& subId)
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

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
void vtkPolyLine::Derivatives(int subId, double pcoords[3], double *values, 
                              int dim, double *derivs)
{
  this->Line->PointIds->SetNumberOfIds(2);

  this->Line->Points->SetPoint(0,this->Points->GetPoint(subId));
  this->Line->Points->SetPoint(1,this->Points->GetPoint(subId+1));

  this->Line->Derivatives(0, pcoords, values+dim*subId, dim, derivs);
}

//----------------------------------------------------------------------------
void vtkPolyLine::Clip(double value, vtkDataArray *cellScalars, 
                       vtkIncrementalPointLocator *locator, vtkCellArray *lines,
                       vtkPointData *inPd, vtkPointData *outPd,
                       vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd,
                       int insideOut)
{
  int i, numLines=this->Points->GetNumberOfPoints() - 1;
  vtkDoubleArray *lineScalars=vtkDoubleArray::New();
  lineScalars->SetNumberOfTuples(2);

  for ( i=0; i < numLines; i++)
    {
    this->Line->Points->SetPoint(0,this->Points->GetPoint(i));
    this->Line->Points->SetPoint(1,this->Points->GetPoint(i+1));

    this->Line->PointIds->SetId(0,this->PointIds->GetId(i));
    this->Line->PointIds->SetId(1,this->PointIds->GetId(i+1));

    lineScalars->SetComponent(0,0,cellScalars->GetComponent(i,0));
    lineScalars->SetComponent(1,0,cellScalars->GetComponent(i+1,0));

    this->Line->Clip(value, lineScalars, locator, lines, inPd, outPd, 
                    inCd, cellId, outCd, insideOut);
    }
  
  lineScalars->Delete();
}

//----------------------------------------------------------------------------
// Return the center of the point cloud in parametric coordinates.
int vtkPolyLine::GetParametricCenter(double pcoords[3])
{
  pcoords[0] = 0.5; pcoords[1] = pcoords[2] = 0.0;
  return ((this->Points->GetNumberOfPoints() - 1) / 2);
}

//----------------------------------------------------------------------------
void vtkPolyLine::InterpolateFunctions(double pcoords[3], double *weights)
{
  (void)pcoords;
  (void)weights;
}

//----------------------------------------------------------------------------
void vtkPolyLine::InterpolateDerivs(double pcoords[3], double *derivs)
{
  (void)pcoords;
  (void)derivs;
}

//----------------------------------------------------------------------------
void vtkPolyLine::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "Line:\n";
  this->Line->PrintSelf(os,indent.GetNextIndent());
}

