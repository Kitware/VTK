/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWedge.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkWedge.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkIncrementalPointLocator.h"
#include "vtkQuad.h"
#include "vtkTriangle.h"
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkWedge);

static const double VTK_DIVERGED = 1.e6;

//----------------------------------------------------------------------------
// Construct the wedge with six points.
vtkWedge::vtkWedge()
{
  this->Points->SetNumberOfPoints(6);
  this->PointIds->SetNumberOfIds(6);

  for (int i = 0; i < 6; i++)
    {
    this->Points->SetPoint(i, 0.0, 0.0, 0.0);
    this->PointIds->SetId(i,0);
    }

  this->Line = vtkLine::New();
  this->Triangle = vtkTriangle::New();
  this->Quad = vtkQuad::New();
}

//----------------------------------------------------------------------------
vtkWedge::~vtkWedge()
{
  this->Line->Delete();
  this->Triangle->Delete();
  this->Quad->Delete();
}


static const int VTK_WEDGE_MAX_ITERATION=10;
static const double VTK_WEDGE_CONVERGED=1.e-03;

//----------------------------------------------------------------------------
int vtkWedge::EvaluatePosition(double x[3], double* closestPoint,
                               int& subId, double pcoords[3],
                               double& dist2, double *weights)
{
  int iteration, converged;
  double  params[3];
  double  fcol[3], rcol[3], scol[3], tcol[3];
  int i, j;
  double  d, pt[3];
  double derivs[18];

  //  set initial position for Newton's method
  subId = 0;
  pcoords[0] = pcoords[1] = pcoords[2] = 0.5;
  params[0] = params[1] = params[2] = 0.5;

  //  enter iteration loop
  for (iteration=converged=0; !converged && (iteration < VTK_WEDGE_MAX_ITERATION);
  iteration++)
    {
    //  calculate element interpolation functions and derivatives
    this->InterpolationFunctions(pcoords, weights);
    this->InterpolationDerivs(pcoords, derivs);

    //  calculate newton functions
    for (i=0; i<3; i++)
      {
      fcol[i] = rcol[i] = scol[i] = tcol[i] = 0.0;
      }
    for (i=0; i<6; i++)
      {
      this->Points->GetPoint(i, pt);
      for (j=0; j<3; j++)
        {
        fcol[j] += pt[j] * weights[i];
        rcol[j] += pt[j] * derivs[i];
        scol[j] += pt[j] * derivs[i+6];
        tcol[j] += pt[j] * derivs[i+12];
        }
      }

    for (i=0; i<3; i++)
      {
      fcol[i] -= x[i];
      }

    //  compute determinants and generate improvements
    d=vtkMath::Determinant3x3(rcol,scol,tcol);
    if ( fabs(d) < 1.e-20)
      {
      return -1;
      }

    pcoords[0] = params[0] - vtkMath::Determinant3x3 (fcol,scol,tcol) / d;
    pcoords[1] = params[1] - vtkMath::Determinant3x3 (rcol,fcol,tcol) / d;
    pcoords[2] = params[2] - vtkMath::Determinant3x3 (rcol,scol,fcol) / d;

    //  check for convergence
    if ( ((fabs(pcoords[0]-params[0])) < VTK_WEDGE_CONVERGED) &&
    ((fabs(pcoords[1]-params[1])) < VTK_WEDGE_CONVERGED) &&
    ((fabs(pcoords[2]-params[2])) < VTK_WEDGE_CONVERGED) )
      {
      converged = 1;
      }
    // Test for bad divergence (S.Hirschberg 11.12.2001)
    else if ((fabs(pcoords[0]) > VTK_DIVERGED) ||
             (fabs(pcoords[1]) > VTK_DIVERGED) ||
             (fabs(pcoords[2]) > VTK_DIVERGED))
      {
      return -1;
      }
    //  if not converged, repeat
    else
      {
      params[0] = pcoords[0];
      params[1] = pcoords[1];
      params[2] = pcoords[2];
      }
    }

  //  if not converged, set the parametric coordinates to arbitrary values
  //  outside of element
  if ( !converged )
    {
    return -1;
    }

  this->InterpolationFunctions(pcoords, weights);

  if ( pcoords[0] >= -0.001 && pcoords[0] <= 1.001 &&
  pcoords[1] >= -0.001 && pcoords[1] <= 1.001 &&
  pcoords[2] >= -0.001 && pcoords[2] <= 1.001 )
    {
    if (closestPoint)
      {
      closestPoint[0] = x[0]; closestPoint[1] = x[1]; closestPoint[2] = x[2];
      dist2 = 0.0; //inside wedge
      }
    return 1;
    }
  else
    {
    double pc[3], w[6];
    if (closestPoint)
      {
      for (i=0; i<3; i++) //only approximate, not really true for warped hexa
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
      this->EvaluateLocation(subId, pc, closestPoint,
                             static_cast<double *>(w));
      dist2 = vtkMath::Distance2BetweenPoints(closestPoint,x);
      }
    return 0;
    }
}

//----------------------------------------------------------------------------
void vtkWedge::EvaluateLocation(int& vtkNotUsed(subId), double pcoords[3],
                                double x[3], double *weights)
{
  int i, j;
  double pt[3];

  this->InterpolationFunctions(pcoords, weights);

  x[0] = x[1] = x[2] = 0.0;
  for (i=0; i<6; i++)
    {
    this->Points->GetPoint(i, pt);
    for (j=0; j<3; j++)
      {
      x[j] += pt[j] * weights[i];
      }
    }
}

//----------------------------------------------------------------------------
// Returns the closest face to the point specified. Closeness is measured
// parametrically.
int vtkWedge::CellBoundary(int vtkNotUsed(subId), double pcoords[3],
                           vtkIdList *pts)
{
  int i;

  // define 9 planes that separate regions
  static double normals[9][3] = {
    {0.0,0.83205,-0.5547}, {-0.639602,-0.639602,-0.426401}, {0.83205,0.0,-0.5547},
    {0.0,0.83205,0.5547}, {-0.639602,-0.639602,0.426401}, {0.83205,0.0,0.5547},
    {-0.707107,0.707107,0.0}, {0.447214,0.894427,0.0}, {0.894427,0.447214,0.0} };
  static double point[3] = {0.333333,0.333333,0.5};
  double vals[9];

  // evaluate 9 plane equations
  for (i=0; i<9; i++)
    {
    vals[i] = normals[i][0]*(pcoords[0]-point[0]) +
      normals[i][1]*(pcoords[1]-point[1]) + normals[i][2]*(pcoords[2]-point[2]);
    }

  // compare against nine planes in parametric space that divide element
  // into five pieces (each corresponding to a face).
  if ( vals[0] >= 0.0 && vals[1] >= 0.0 && vals[2] >= 0.0 )
    {
    pts->SetNumberOfIds(3); //triangle face
    pts->SetId(0,this->PointIds->GetId(0));
    pts->SetId(1,this->PointIds->GetId(1));
    pts->SetId(2,this->PointIds->GetId(2));
    }

  else if ( vals[3] >= 0.0 && vals[4] >= 0.0 && vals[5] >= 0.0 )
    {
    pts->SetNumberOfIds(3); //triangle face
    pts->SetId(0,this->PointIds->GetId(3));
    pts->SetId(1,this->PointIds->GetId(4));
    pts->SetId(2,this->PointIds->GetId(5));
    }

  else if ( vals[0] <= 0.0 && vals[3] <= 0.0 &&
            vals[6] <= 0.0 && vals[7] <= 0.0 )
    {
    pts->SetNumberOfIds(4); //quad face
    pts->SetId(0,this->PointIds->GetId(0));
    pts->SetId(1,this->PointIds->GetId(1));
    pts->SetId(2,this->PointIds->GetId(4));
    pts->SetId(3,this->PointIds->GetId(3));
    }

  else if ( vals[1] <= 0.0 && vals[4] <= 0.0 &&
            vals[7] >= 0.0 && vals[8] >= 0.0 )
    {
    pts->SetNumberOfIds(4); //quad face
    pts->SetId(0,this->PointIds->GetId(1));
    pts->SetId(1,this->PointIds->GetId(2));
    pts->SetId(2,this->PointIds->GetId(5));
    pts->SetId(3,this->PointIds->GetId(4));
    }

  else //vals[2] <= 0.0 && vals[5] <= 0.0 && vals[8] <= 0.0 && vals[6] >= 0.0
    {
    pts->SetNumberOfIds(4); //quad face
    pts->SetId(0,this->PointIds->GetId(2));
    pts->SetId(1,this->PointIds->GetId(0));
    pts->SetId(2,this->PointIds->GetId(3));
    pts->SetId(3,this->PointIds->GetId(5));
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

//----------------------------------------------------------------------------
// Marching (convex) wedge
//
static int edges[9][2] = { {0,1}, {1,2}, {2,0},
                           {3,4}, {4,5}, {5,3},
                           {0,3}, {1,4}, {2,5} };
static int faces[5][4] = { {0,1,2,-1}, {3,5,4,-1},
                           {0,3,4,1}, {1,4,5,2}, {2,5,3,0} };

typedef int EDGE_LIST;
typedef struct {
       EDGE_LIST edges[13];
} TRIANGLE_CASES;

static TRIANGLE_CASES triCases[] = {
  {{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, //0
  {{ 0,  6,  2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, //1
  {{ 0,  1,  7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, //2
  {{ 6,  1,  7,  6,  2,  1, -1, -1, -1, -1, -1, -1, -1}}, //3
  {{ 1,  2,  8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, //4
  {{ 6,  1,  0,  6,  8,  1, -1, -1, -1, -1, -1, -1, -1}}, //5
  {{ 0,  2,  8,  7,  0,  8, -1, -1, -1, -1, -1, -1, -1}}, //6
  {{ 7,  6,  8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, //7
  {{ 3,  5,  6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, //8
  {{ 3,  5,  0,  5,  2,  0, -1, -1, -1, -1, -1, -1, -1}}, //9
  {{ 0,  1,  7,  6,  3,  5, -1, -1, -1, -1, -1, -1, -1}}, //10
  {{ 1,  7,  3,  1,  3,  5,  1,  5,  2, -1, -1, -1, -1}}, //11
  {{ 2,  8,  1,  6,  3,  5, -1, -1, -1, -1, -1, -1, -1}}, //12
  {{ 0,  3,  1,  1,  3,  5,  1,  5,  8, -1, -1, -1, -1}}, //13
  {{ 6,  3,  5,  0,  8,  7,  0,  2,  8, -1, -1, -1, -1}}, //14
  {{ 7,  3,  5,  7,  5,  8, -1, -1, -1, -1, -1, -1, -1}}, //15
  {{ 7,  4,  3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, //16
  {{ 7,  4,  3,  0,  6,  2, -1, -1, -1, -1, -1, -1, -1}}, //17
  {{ 0,  1,  3,  1,  4,  3, -1, -1, -1, -1, -1, -1, -1}}, //18
  {{ 1,  4,  3,  1,  3,  6,  1,  6,  2, -1, -1, -1, -1}}, //19
  {{ 7,  4,  3,  2,  8,  1, -1, -1, -1, -1, -1, -1, -1}}, //20
  {{ 7,  4,  3,  6,  1,  0,  6,  8,  1, -1, -1, -1, -1}}, //21
  {{ 0,  4,  3,  0,  8,  4,  0,  2,  8, -1, -1, -1, -1}}, //22
  {{ 6,  8,  3,  3,  8,  4, -1, -1, -1, -1, -1, -1, -1}}, //23
  {{ 6,  7,  4,  6,  4,  5, -1, -1, -1, -1, -1, -1, -1}}, //24
  {{ 0,  7,  5,  7,  4,  5,  2,  0,  5, -1, -1, -1, -1}}, //25
  {{ 1,  6,  0,  1,  5,  6,  1,  4,  5, -1, -1, -1, -1}}, //26
  {{ 2,  1,  5,  5,  1,  4, -1, -1, -1, -1, -1, -1, -1}}, //27
  {{ 2,  8,  1,  6,  7,  5,  7,  4,  5, -1, -1, -1, -1}}, //28
  {{ 0,  7,  5,  7,  4,  5,  0,  5,  1,  1,  5,  8, -1}}, //29
  {{ 0,  2,  8,  0,  8,  4,  0,  4,  5,  0,  5,  6, -1}}, //30
  {{ 8,  4,  5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, //31
  {{ 4,  8,  5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, //32
  {{ 4,  8,  5,  0,  6,  2, -1, -1, -1, -1, -1, -1, -1}}, //33
  {{ 4,  8,  5,  0,  1,  7, -1, -1, -1, -1, -1, -1, -1}}, //34
  {{ 4,  8,  5,  6,  1,  7,  6,  2,  1, -1, -1, -1, -1}}, //35
  {{ 1,  5,  4,  2,  5,  1, -1, -1, -1, -1, -1, -1, -1}}, //36
  {{ 1,  5,  4,  1,  6,  5,  1,  0,  6, -1, -1, -1, -1}}, //37
  {{ 5,  4,  7,  5,  7,  0,  5,  0,  2, -1, -1, -1, -1}}, //38
  {{ 6,  4,  7,  6,  5,  4, -1, -1, -1, -1, -1, -1, -1}}, //39
  {{ 6,  3,  8,  3,  4,  8, -1, -1, -1, -1, -1, -1, -1}}, //40
  {{ 0,  3,  4,  0,  4,  8,  0,  8,  2, -1, -1, -1, -1}}, //41
  {{ 7,  0,  1,  6,  3,  4,  6,  4,  8, -1, -1, -1, -1}}, //42
  {{ 1,  7,  3,  1,  3,  2,  2,  3,  8,  8,  3,  4, -1}}, //43
  {{ 2,  6,  1,  6,  3,  1,  3,  4,  1, -1, -1, -1, -1}}, //44
  {{ 0,  3,  1,  1,  3,  4, -1, -1, -1, -1, -1, -1, -1}}, //45
  {{ 7,  0,  4,  4,  0,  2,  4,  2,  3,  3,  2,  6, -1}}, //46
  {{ 7,  3,  4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, //47
  {{ 7,  8,  5,  7,  5,  3, -1, -1, -1, -1, -1, -1, -1}}, //48
  {{ 0,  6,  2,  7,  8,  5,  7,  5,  3, -1, -1, -1, -1}}, //49
  {{ 0,  1,  3,  1,  5,  3,  1,  8,  5, -1, -1, -1, -1}}, //50
  {{ 2,  1,  6,  6,  1,  3,  5,  1,  8,  3,  1,  5, -1}}, //51
  {{ 1,  3,  7,  1,  5,  3,  1,  2,  5, -1, -1, -1, -1}}, //52
  {{ 1,  0,  6,  1,  6,  5,  1,  5,  7,  7,  5,  3, -1}}, //53
  {{ 0,  2,  5,  0,  5,  3, -1, -1, -1, -1, -1, -1, -1}}, //54
  {{ 3,  6,  5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, //55
  {{ 7,  8,  6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, //56
  {{ 0,  7,  8,  0,  8,  2, -1, -1, -1, -1, -1, -1, -1}}, //57
  {{ 0,  1,  6,  1,  8,  6, -1, -1, -1, -1, -1, -1, -1}}, //58
  {{ 2,  1,  8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, //59
  {{ 6,  7,  1,  6,  1,  2, -1, -1, -1, -1, -1, -1, -1}}, //60
  {{ 0,  7,  1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, //61
  {{ 0,  2,  6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, //62
  {{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}  //63
};

//----------------------------------------------------------------------------
void vtkWedge::Contour(double value, vtkDataArray *cellScalars,
                       vtkIncrementalPointLocator *locator,
                       vtkCellArray *verts,
                       vtkCellArray *lines,
                       vtkCellArray *polys,
                       vtkPointData *inPd, vtkPointData *outPd,
                       vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd)
{
  static int CASE_MASK[6] = {1,2,4,8,16,32};
  TRIANGLE_CASES *triCase;
  EDGE_LIST  *edge;
  int i, j, index, *vert, v1, v2, newCellId;
  vtkIdType pts[3];
  double t, x1[3], x2[3], x[3], deltaScalar;
  vtkIdType offset = verts->GetNumberOfCells() + lines->GetNumberOfCells();

  // Build the case table
  for ( i=0, index = 0; i < 6; i++)
    {
    if (cellScalars->GetComponent(i,0) >= value)
      {
      index |= CASE_MASK[i];
      }
    }

  triCase = triCases + index;
  edge = triCase->edges;

  for ( ; edge[0] > -1; edge += 3 )
    {
    for (i=0; i<3; i++) // insert triangle
      {
      vert = edges[edge[i]];

      // calculate a preferred interpolation direction
      deltaScalar = (cellScalars->GetComponent(vert[1],0)
                     - cellScalars->GetComponent(vert[0],0));
      if (deltaScalar > 0)
        {
        v1 = vert[0]; v2 = vert[1];
        }
      else
        {
        v1 = vert[1]; v2 = vert[0];
        deltaScalar = -deltaScalar;
        }

      // linear interpolation
      t = ( deltaScalar == 0.0 ? 0.0 :
            (value - cellScalars->GetComponent(v1,0)) / deltaScalar );

      this->Points->GetPoint(v1, x1);
      this->Points->GetPoint(v2, x2);

      for (j=0; j<3; j++)
        {
        x[j] = x1[j] + t * (x2[j] - x1[j]);
        }
      if ( locator->InsertUniquePoint(x, pts[i]) )
        {
        if ( outPd )
          {
          vtkIdType p1 = this->PointIds->GetId(v1);
          vtkIdType p2 = this->PointIds->GetId(v2);
          outPd->InterpolateEdge(inPd,pts[i],p1,p2,t);
          }
        }
      }
    // check for degenerate triangle
    if ( pts[0] != pts[1] && pts[0] != pts[2] && pts[1] != pts[2] )
      {
      newCellId = offset + polys->InsertNextCell(3,pts);
      outCd->CopyData(inCd,cellId,newCellId);
      }
    }
}

//----------------------------------------------------------------------------
int *vtkWedge::GetEdgeArray(int edgeId)
{
  return edges[edgeId];
}

//----------------------------------------------------------------------------
vtkCell *vtkWedge::GetEdge(int edgeId)
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

//----------------------------------------------------------------------------
int *vtkWedge::GetFaceArray(int faceId)
{
  return faces[faceId];
}

//----------------------------------------------------------------------------
vtkCell *vtkWedge::GetFace(int faceId)
{
  int *verts = faces[faceId];

  if ( verts[3] != -1 ) // quad cell
    {
    // load point id's
    this->Quad->PointIds->SetId(0,this->PointIds->GetId(verts[0]));
    this->Quad->PointIds->SetId(1,this->PointIds->GetId(verts[1]));
    this->Quad->PointIds->SetId(2,this->PointIds->GetId(verts[2]));
    this->Quad->PointIds->SetId(3,this->PointIds->GetId(verts[3]));

    // load coordinates
    this->Quad->Points->SetPoint(0,this->Points->GetPoint(verts[0]));
    this->Quad->Points->SetPoint(1,this->Points->GetPoint(verts[1]));
    this->Quad->Points->SetPoint(2,this->Points->GetPoint(verts[2]));
    this->Quad->Points->SetPoint(3,this->Points->GetPoint(verts[3]));

    return this->Quad;
    }
  else
    {
    // load point id's
    this->Triangle->PointIds->SetId(0,this->PointIds->GetId(verts[0]));
    this->Triangle->PointIds->SetId(1,this->PointIds->GetId(verts[1]));
    this->Triangle->PointIds->SetId(2,this->PointIds->GetId(verts[2]));

    // load coordinates
    this->Triangle->Points->SetPoint(0,this->Points->GetPoint(verts[0]));
    this->Triangle->Points->SetPoint(1,this->Points->GetPoint(verts[1]));
    this->Triangle->Points->SetPoint(2,this->Points->GetPoint(verts[2]));

    return this->Triangle;
    }
}

//----------------------------------------------------------------------------
// Intersect faces against line.
//
int vtkWedge::IntersectWithLine(double p1[3], double p2[3],
                                double tol, double& t,
                                double x[3], double pcoords[3], int& subId)
{
  int intersection=0;
  double pt1[3], pt2[3], pt3[3], pt4[3];
  double tTemp;
  double pc[3], xTemp[3];
  int faceNum;

  t = VTK_DOUBLE_MAX;

  //first intersect the triangle faces
  for (faceNum=0; faceNum<2; faceNum++)
    {
    this->Points->GetPoint(faces[faceNum][0], pt1);
    this->Points->GetPoint(faces[faceNum][1], pt2);
    this->Points->GetPoint(faces[faceNum][2], pt3);

    this->Triangle->Points->SetPoint(0,pt1);
    this->Triangle->Points->SetPoint(1,pt2);
    this->Triangle->Points->SetPoint(2,pt3);

    if ( this->Triangle->IntersectWithLine(p1, p2, tol, tTemp, xTemp,
                                           pc, subId) )
      {
      intersection = 1;
      if ( tTemp < t )
        {
        t = tTemp;
        x[0] = xTemp[0]; x[1] = xTemp[1]; x[2] = xTemp[2];
        switch (faceNum)
          {
          case 0:
            pcoords[0] = pc[0]; pcoords[1] = pc[1]; pcoords[2] = 0.0;
            break;

          case 1:
            pcoords[0] = pc[0]; pcoords[1] = pc[1]; pcoords[2] = 1.0;
            break;
          }
        }
      }
    }

  //now intersect the quad faces
  for (faceNum=2; faceNum<5; faceNum++)
    {
    this->Points->GetPoint(faces[faceNum][0], pt1);
    this->Points->GetPoint(faces[faceNum][1], pt2);
    this->Points->GetPoint(faces[faceNum][2], pt3);
    this->Points->GetPoint(faces[faceNum][3], pt4);

    this->Quad->Points->SetPoint(0,pt1);
    this->Quad->Points->SetPoint(1,pt2);
    this->Quad->Points->SetPoint(2,pt3);
    this->Quad->Points->SetPoint(3,pt4);

    if ( this->Quad->IntersectWithLine(p1, p2, tol, tTemp, xTemp, pc, subId) )
      {
      intersection = 1;
      if ( tTemp < t )
        {
        t = tTemp;
        x[0] = xTemp[0]; x[1] = xTemp[1]; x[2] = xTemp[2];
        switch (faceNum)
          {
          case 2:
            pcoords[0] = pc[1]; pcoords[1] = 0.0; pcoords[2] = pc[0];
            break;

          case 3:
            pcoords[0] = 1.0-pc[1]; pcoords[1] = pc[1]; pcoords[2] = pc[0];
            break;

          case 4:
            pcoords[0] = 0.0; pcoords[1] = pc[1]; pcoords[2] = pc[0];
            break;
          }
        }
      }
    }

  return intersection;
}

//----------------------------------------------------------------------------
int vtkWedge::Triangulate(int vtkNotUsed(index), vtkIdList *ptIds,
                          vtkPoints *pts)
{
  ptIds->Reset();
  pts->Reset();

  // one wedge (or prism) is decomposed into 3 tetrahedrons and four
  // pairs of (pointId, pointCoordinates) are provided for each tetrahedron

  int i, p[4];

  // Tetra #0 info (original point Ids): { 0, 2, 1, 3 }
  p[0] = 0; p[1] = 2; p[2] = 1; p[3] = 3;
  for ( i=0; i < 4; i++ )
    {
    ptIds->InsertNextId(this->PointIds->GetId(p[i]));
    pts->InsertNextPoint(this->Points->GetPoint(p[i]));
    }

  // Tetra #1 info (original point Ids): { 1, 3, 5, 4 }
  p[0] = 1; p[1] = 3; p[2] = 5; p[3] = 4;
  for ( i=0; i < 4; i++ )
    {
    ptIds->InsertNextId(this->PointIds->GetId(p[i]));
    pts->InsertNextPoint(this->Points->GetPoint(p[i]));
    }

  // Tetra #2 info (original point Ids): { 1, 2, 5, 3 }
  p[0] = 1; p[1] = 2; p[2] = 5; p[3] = 3;
  for ( i=0; i < 4; i++ )
    {
    ptIds->InsertNextId(this->PointIds->GetId(p[i]));
    pts->InsertNextPoint(this->Points->GetPoint(p[i]));
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkWedge::Derivatives(int vtkNotUsed(subId), double pcoords[3],
                           double *values, int dim, double *derivs)
{
  double *jI[3], j0[3], j1[3], j2[3];
  double functionDerivs[18], sum[3], value;
  int i, j, k;

  // compute inverse Jacobian and interpolation function derivatives
  jI[0] = j0; jI[1] = j1; jI[2] = j2;
  this->JacobianInverse(pcoords, jI, functionDerivs);

  // now compute derivates of values provided
  for (k=0; k < dim; k++) //loop over values per vertex
    {
    sum[0] = sum[1] = sum[2] = 0.0;
    for ( i=0; i < 6; i++) //loop over interp. function derivatives
      {
      value = values[dim*i + k];
      sum[0] += functionDerivs[i] * value;
      sum[1] += functionDerivs[6 + i] * value;
      sum[2] += functionDerivs[12 + i] * value;
      }

    for (j=0; j < 3; j++) //loop over derivative directions
      {
      derivs[3*k + j] = sum[0]*jI[j][0] + sum[1]*jI[j][1] + sum[2]*jI[j][2];
      }
    }
}

//----------------------------------------------------------------------------
// Compute iso-parametric interpolation functions
//
void vtkWedge::InterpolationFunctions(double pcoords[3], double sf[6])
{
  sf[0] = (1.0 - pcoords[0] - pcoords[1]) * (1.0 - pcoords[2]);
  sf[1] = pcoords[0] * (1.0 - pcoords[2]);
  sf[2] = pcoords[1] * (1.0 - pcoords[2]);
  sf[3] = (1.0 - pcoords[0] - pcoords[1]) * pcoords[2];
  sf[4] = pcoords[0] * pcoords[2];
  sf[5] = pcoords[1] * pcoords[2];
}

//----------------------------------------------------------------------------
void vtkWedge::InterpolationDerivs(double pcoords[3], double derivs[18])
{
  // r-derivatives
  derivs[0] = -1.0 + pcoords[2];
  derivs[1] =  1.0 - pcoords[2];
  derivs[2] =  0.0;
  derivs[3] = -pcoords[2];
  derivs[4] =  pcoords[2];
  derivs[5] =  0.0;

  // s-derivatives
  derivs[6] = -1.0 + pcoords[2];
  derivs[7] =  0.0;
  derivs[8] =  1.0 - pcoords[2];
  derivs[9] = -pcoords[2];
  derivs[10] = 0.0;
  derivs[11] = pcoords[2];

  // t-derivatives
  derivs[12] = -1.0 + pcoords[0] + pcoords[1];
  derivs[13] = -pcoords[0];
  derivs[14] = -pcoords[1];
  derivs[15] =  1.0 - pcoords[0] - pcoords[1];
  derivs[16] =  pcoords[0];
  derivs[17] =  pcoords[1];
}

//----------------------------------------------------------------------------
// Given parametric coordinates compute inverse Jacobian transformation
// matrix. Returns 9 elements of 3x3 inverse Jacobian plus interpolation
// function derivatives. Returns 0 if no inverse exists.
int vtkWedge::JacobianInverse(double pcoords[3], double **inverse,
                              double derivs[18])
{
  int i, j;
  double *m[3], m0[3], m1[3], m2[3];
  double x[3];

  // compute interpolation function derivatives
  this->InterpolationDerivs(pcoords,derivs);

  // create Jacobian matrix
  m[0] = m0; m[1] = m1; m[2] = m2;
  for (i=0; i < 3; i++) //initialize matrix
    {
    m0[i] = m1[i] = m2[i] = 0.0;
    }

  for ( j=0; j < 6; j++ )
    {
    this->Points->GetPoint(j, x);
    for ( i=0; i < 3; i++ )
      {
      m0[i] += x[i] * derivs[j];
      m1[i] += x[i] * derivs[6 + j];
      m2[i] += x[i] * derivs[12 + j];
      }
    }

  // now find the inverse
  if ( vtkMath::InvertMatrix(m,inverse,3) == 0 )
    {
#define VTK_MAX_WARNS 3
    static int numWarns=0;
    if ( numWarns++ < VTK_MAX_WARNS )
      {
      vtkErrorMacro(<<"Jacobian inverse not found");
      vtkErrorMacro(<<"Matrix:" << m[0][0] << " " << m[0][1] << " " << m[0][2]
      << m[1][0] << " " << m[1][1] << " " << m[1][2]
      << m[2][0] << " " << m[2][1] << " " << m[2][2] );
      return 0;
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkWedge::GetEdgePoints(int edgeId, int* &pts)
{
  pts = this->GetEdgeArray(edgeId);
}

//----------------------------------------------------------------------------
void vtkWedge::GetFacePoints(int faceId, int* &pts)
{
  pts = this->GetFaceArray(faceId);
}

static double vtkWedgeCellPCoords[18] = {0.0,0.0,0.0, 1.0,0.0,0.0, 0.0,1.0,0.0,
                                         0.0,0.0,1.0, 1.0,0.0,1.0, 0.0,1.0,1.0};

//----------------------------------------------------------------------------
double *vtkWedge::GetParametricCoords()
{
  return vtkWedgeCellPCoords;
}

//----------------------------------------------------------------------------
void vtkWedge::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Line:\n";
  this->Line->PrintSelf(os,indent.GetNextIndent());
  os << indent << "Triangle:\n";
  this->Triangle->PrintSelf(os,indent.GetNextIndent());
  os << indent << "Quad:\n";
  this->Quad->PrintSelf(os,indent.GetNextIndent());
}
