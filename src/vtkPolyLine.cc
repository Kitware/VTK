/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyLine.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkPolyLine.hh"
#include "vtkMath.hh"
#include "vtkLine.hh"
#include "vtkCellArray.hh"

//
// eliminate constructor / destructor calls
//
static vtkMath math;


// Description:
// Deep copy of cell.
vtkPolyLine::vtkPolyLine(const vtkPolyLine& pl)
{
  this->Points = pl.Points;
  this->PointIds = pl.PointIds;
}

// Description:
// Given points and lines, compute normals to lines.
int vtkPolyLine::GenerateNormals(vtkPoints *pts, vtkCellArray *lines, vtkFloatNormals *normals)
{
  int npts, *linePts;
  float s[3], sPrev[3], sNext[3], norm[3], *n, *nPrev;
  float *p, *pPrev, *pNext;
  float n_norm;
  int i, j, fillIn;
  int aNormalComputed, *normalComputed;
//
//  Loop over all lines. 
// 
  for (lines->InitTraversal(); lines->GetNextCell(npts,linePts); )
    {

    // check input
    if ( npts < 1 )
      {
      vtkErrorMacro(<<"Line with no points!");
      return 0;
      }

    else if ( npts == 1 ) //return arbitrary normal
      {
      norm[0] = norm[1] = 0.0;
      norm[2] = 1.0;
      normals->InsertNormal(linePts[0],norm);
      return 1;
      }

    else if ( npts == 2 ) //simple line; directly compute
      {
      pPrev = pts->GetPoint(linePts[0]);
      p = pts->GetPoint(linePts[1]);

      for (i=0; i<3; i++) s[i] = p[i] - pPrev[i];

      if ( (n_norm = math.Norm(s)) == 0.0 ) //use arbitrary normal
        {
        norm[0] = norm[1] = 0.0;
        norm[2] = 1.0;
        }

      else //else compute normal
        {
        for (i=0; i<3; i++) 
          {
          if ( s[i] != 0.0 ) 
            {
            norm[(i+2)%3] = 0.0;
            norm[(i+1)%3] = 1.0;
            norm[i] = -s[(i+1)%3]/s[i];
            break;
            }
          }
        n_norm = math.Norm(norm);
        for (i=0; i<3; i++) norm[i] /= n_norm;
        }

      normals->InsertNormal(linePts[0],norm);
      normals->InsertNormal(linePts[1],norm);
      return 1;
      }
//
//  Else have polyline. Initialize normal computation.
//
    normalComputed = new int[npts+1];
    for (i=0; i<=npts; i++) normalComputed[i] = 0;
    p = pts->GetPoint(linePts[0]);
    pNext = pts->GetPoint(linePts[1]);

    // perform cross products along line
    for (aNormalComputed=0, j=1; j < (npts-1); j++) 
      {
      pPrev = p;
      p = pNext;
      pNext = pts->GetPoint(linePts[j+1]);

      for (i=0; i<3; i++) 
        {
        sPrev[i] = p[i] - pPrev[i];
        sNext[i] = pNext[i] - p[i];
        }

      math.Cross(sPrev,sNext,norm);
      if ( (n_norm = math.Norm(norm)) != 0.0 ) //okay to use
        {
        for (i=0; i<3; i++) norm[i] /= n_norm;
        normalComputed[j] = aNormalComputed = 1;
        normals->InsertNormal(linePts[j],norm);
        }
      }
//
//  If no normal computed, must be straight line of points. Find one normal.
//
    if ( ! aNormalComputed )
      {
      for (j=1; j < npts; j++) 
        {
        pPrev = pts->GetPoint(linePts[j-1]);
        p = pts->GetPoint(linePts[j]);

        for (i=0; i<3; i++) s[i] = p[i] - pPrev[i];

        if ( (n_norm = math.Norm(s)) != 0.0 ) //okay to use
          {
          aNormalComputed = 1;

          for (i=0; i<3; i++) 
            {
            if ( s[i] != 0.0 ) 
              {
              norm[(i+2)%3] = 0.0;
              norm[(i+1)%3] = 1.0;
              norm[i] = -s[(i+1)%3]/s[i];
              break;
              }
            }
          n_norm = math.Norm(norm);
          for (i=0; i<3; i++) norm[i] /= n_norm;

          break;
          }
        }

      if ( ! aNormalComputed ) // must be a bunch of coincident points
        {
        norm[0] = norm[1] = 0.0;
        norm[2] = 1.0;
        }

      for (j=0; j<npts; j++) normals->InsertNormal(linePts[j],norm);
      return 1;
      }
//
//  Fill in normals (from neighbors)
//
    for (fillIn=1; fillIn ; )
      {
      for (fillIn=0, j=0; j<npts; j++)
        {
        if ( ! normalComputed[j] )
          {
          if ( (j+1) < npts && normalComputed[j+1] )
            {
            fillIn = 1;
            normals->InsertNormal(linePts[j],normals->GetNormal(j+1));
            normalComputed[j] = 1;
            }
          else if ( (j-1) >= 0 && normalComputed[j-1] )
            {
            fillIn = 1;
            normals->InsertNormal(linePts[j],normals->GetNormal(j-1));
            normalComputed[j] = 1;
            }
          }
        }
      }
    delete [] normalComputed;
//
//  Check that normals don't flip around wildly.
//
    n = normals->GetNormal(linePts[0]);
    for (j=1; j<npts; j++)
      {
      nPrev = n;
      n = normals->GetNormal(linePts[j]);

      if ( math.Dot(n,nPrev) < 0.0 ) //reversed sense
        {
        for(i=0; i<3; i++) norm[i] = -n[i];
        normals->InsertNormal(linePts[j],norm);
        }
      }
    }

  return 1;
}

// Description:
// Given points and lines, compute normals to lines. These are not true 
// normals, they are "orientation" normals used by classes like vtkTubeFilter
// that control the rotation around the line. The normals try to stay pointing
// in the same direction as much as possible (i.e., minimal rotation).
int vtkPolyLine::GenerateSlidingNormals(vtkPoints *pts, vtkCellArray *lines, vtkFloatNormals *normals)
{
  int npts, *linePts;
  float sPrev[3], sNext[3], q[3], w[3], normal[3], theta;
  float p[3], pPrev[3], pNext[3];
  int i, j, largeRotation;
//
//  Loop over all lines
// 
  for (lines->InitTraversal(); lines->GetNextCell(npts,linePts); )
    {
//
//  Determine initial starting normal
// 
    if ( npts <= 0 ) continue;

    else if ( npts == 1 ) //return arbitrary
      {
      normal[0] = normal[1] = 0.0;
      normal[2] = 1.0;
      normals->InsertNormal(linePts[0],normal);
      }

    else //more than one point
      {
//
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

          if ( math.Normalize(sNext) == 0.0 )
            {
            vtkErrorMacro(<<"Coincident points in polyline...can't compute normals");
            return 0;
            }

          for (i=0; i<3; i++) 
            {
            if ( sNext[i] != 0.0 ) 
              {
              normal[(i+2)%3] = 0.0;
              normal[(i+1)%3] = 1.0;
              normal[i] = -sNext[(i+1)%3]/sNext[i];
              break;
              }
            }
          math.Normalize(normal);
          normals->InsertNormal(linePts[0],normal);
          }

        else if ( j == (npts-1) ) //last point; just insert previous
          {
          normals->InsertNormal(linePts[j],normal);
          }

        else //inbetween points
          {
//
//  Generate normals for new point by projecting previous normal
// 
          for (i=0; i<3; i++)
            {
            pPrev[i] = p[i];
            p[i] = pNext[i];
            }
          pts->GetPoint(linePts[j+1],pNext);

          for (i=0; i<3; i++) 
            {
            sPrev[i] = sNext[i];
            sNext[i] = pNext[i] - p[i];
            }

          if ( math.Normalize(sNext) == 0.0 )
            {
            vtkErrorMacro(<<"Coincident points in polyline...can't compute normals");
            return 0;
            }

          //compute rotation vector
          math.Cross(sPrev,normal,w);
          if ( math.Normalize(w) == 0.0 ) return 0;

          //see whether we rotate greater than 90 degrees.
          if ( math.Dot(sPrev,sNext) < 0.0 ) largeRotation = 1;
          else largeRotation = 0;

          //compute rotation of line segment
          math.Cross (sNext, sPrev, q);
          if ( (theta=asin((double)math.Normalize(q))) == 0.0 ) 
            { //no rotation, use previous normal
            normals->InsertNormal(linePts[j],normal);
            continue;
            }
          if ( largeRotation )
            {
            if ( theta > 0.0 ) theta = math.Pi() - theta;
            else theta = -math.Pi() - theta;
            }

          //compute projection of rotation of line segment onto rotation vector
          theta *= math.Dot(q,w) / 2.0;

          //compute new normal
          for (i=0; i<3; i++) normal[i] = normal[i]*cos((double)theta) + 
                                          sPrev[i]*sin((double)theta);
          math.Normalize(normal);
          normals->InsertNormal(linePts[j],normal);

          }//for this point
        }//else
      }//else if
    }//for this line
  return 1;

}

int vtkPolyLine::EvaluatePosition(float x[3], float closestPoint[3],
                                 int& subId, float pcoords[3], 
                                 float& minDist2, float *weights)
{
  float closest[3];
  float pc[3], dist2;
  int ignoreId, i, return_status, status;
  float lineWeights[2];
  static vtkLine line;

  pcoords[1] = pcoords[2] = 0.0;

  return_status = 0;
  weights[0] = 0.0;
  for (minDist2=VTK_LARGE_FLOAT,i=0; i<this->Points.GetNumberOfPoints()-1; i++)
    {
    line.Points.SetPoint(0,this->Points.GetPoint(i));
    line.Points.SetPoint(1,this->Points.GetPoint(i+1));
    status = line.EvaluatePosition(x,closest,ignoreId,pc,dist2,lineWeights);
    if ( status != -1 && dist2 < minDist2 )
      {
      return_status = status;
      closestPoint[0] = closest[0]; closestPoint[1] = closest[1]; closestPoint[2] = closest[2]; 
      subId = i;
      pcoords[0] = pc[0];
      minDist2 = dist2;
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
  float *a1 = this->Points.GetPoint(subId);
  float *a2 = this->Points.GetPoint(subId+1);

  for (i=0; i<3; i++) 
    {
    x[i] = a1[i] + pcoords[0]*(a2[i] - a1[i]);
    weights[i] = 0.0;
    }

  weights[subId] = pcoords[0];
  weights[subId+1] = 1.0 - pcoords[0];
}

int vtkPolyLine::CellBoundary(int subId, float pcoords[3], vtkIdList& pts)
{
  pts.Reset();

  if ( pcoords[0] >= 0.5 )
    {
    pts.SetId(0,this->PointIds.GetId(subId+1));
    if ( pcoords[0] > 1.0 ) return 0;
    else return 1;
    }
  else
    {
    pts.SetId(0,this->PointIds.GetId(subId));
    if ( pcoords[0] < 0.0 ) return 0;
    else return 1;
    }
}

void vtkPolyLine::Contour(float value, vtkFloatScalars *cellScalars,
                         vtkPointLocator *locator, vtkCellArray *verts, 
                         vtkCellArray *lines, vtkCellArray *polys, 
                         vtkFloatScalars *scalars)
{
  int i;
  vtkFloatScalars lineScalars(2);
  static vtkLine line;

  for ( i=0; i<this->Points.GetNumberOfPoints()-1; i++)
    {
    line.Points.SetPoint(0,this->Points.GetPoint(i));
    line.Points.SetPoint(1,this->Points.GetPoint(i+1));

    lineScalars.SetScalar(0,cellScalars->GetScalar(i));
    lineScalars.SetScalar(1,cellScalars->GetScalar(i+1));

    line.Contour(value, &lineScalars, locator, verts,
                 lines, polys, scalars);
    }

}

//
// Intersect with sub-lines
//
int vtkPolyLine::IntersectWithLine(float p1[3], float p2[3],float tol,float& t,
                                  float x[3], float pcoords[3], int& subId)
{
  int subTest;
  static vtkLine line;
  for (subId=0; subId<this->Points.GetNumberOfPoints()-1; subId++)
    {
    line.Points.SetPoint(0,this->Points.GetPoint(subId));
    line.Points.SetPoint(1,this->Points.GetPoint(subId+1));

    if ( line.IntersectWithLine(p1, p2, tol, t, x, pcoords, subTest) )
      return 1;
    }

  return 0;
}

int vtkPolyLine::Triangulate(int vtkNotUsed(index), vtkFloatPoints &pts)
{
  pts.Reset();
  for (int subId=0; subId<this->Points.GetNumberOfPoints()-1; subId++)
    {
    pts.InsertPoint(subId,this->Points.GetPoint(subId));
    pts.InsertPoint(subId+1,this->Points.GetPoint(subId+1));
    }

  return 1;
}

void vtkPolyLine::Derivatives(int subId, float pcoords[3], float *values, 
                              int dim, float *derivs)
{
  static vtkLine line;

  line.Points.SetPoint(0,this->Points.GetPoint(subId));
  line.Points.SetPoint(1,this->Points.GetPoint(subId+1));

  line.Derivatives(0, pcoords, values, dim, derivs);
}

