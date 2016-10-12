/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyPlane.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPolyPlane.h"

#include "vtkPolyLine.h"
#include "vtkDoubleArray.h"
#include "vtkLine.h"
#include "vtkPoints.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPolyPlane);
vtkCxxSetObjectMacro( vtkPolyPlane, PolyLine, vtkPolyLine );

//----------------------------------------------------------------------------
vtkPolyPlane::vtkPolyPlane()
{
  this->ExtrusionDirection[0] = 0.0;
  this->ExtrusionDirection[1] = 0.0;
  this->ExtrusionDirection[2] = 1.0;

  this->PolyLine = NULL;
  this->Normals = NULL;
}

//----------------------------------------------------------------------------
vtkPolyPlane::~vtkPolyPlane()
{
  this->SetPolyLine( NULL );

  if (this->Normals)
  {
    this->Normals->Delete();
    this->Normals = NULL;
  }
}

//----------------------------------------------------------------------------
vtkMTimeType vtkPolyPlane::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();

  if (this->PolyLine)
  {
    vtkMTimeType p1Time;
    p1Time = this->PolyLine->GetMTime();
    mTime = ( p1Time > mTime ? p1Time : mTime );
  }

  return mTime;
}

//----------------------------------------------------------------------------
// This function returns 1 if p3 is to the left the directed line from p1 to p2
// and -1 otherwise
// This is computed by testing the determinant:
// | 1 p1[0] p1[1] |
// | 1 p2[0] p2[1] |
// | 1 p3[0] p3[1] |
// which is positive if p3 is to the left of the directed line from p1 to p2,
// zero if p3 is on the line and negative if p3 is to the right of the line.
// Credit: Jack Snoeyink's computational geometry course at UNC
static bool leftOf(double p1[2], double p2[2], double p3[2])
{
  double tmp = p1[0] * p2[1] + p1[1] * p3[0] + p2[0] * p3[1]
             - p1[1] * p2[0] - p3[1] * p1[0] - p3[0] * p2[1];
  return tmp > 0;
}

//----------------------------------------------------------------------------
void vtkPolyPlane::ComputeNormals()
{
  if (!this->PolyLine)
  {
    return;
  }

  if (this->GetMTime() > this->NormalComputeTime.GetMTime())
  {
    // Recompute the normal array.

    if (this->Normals)
    {
      // Delete the array if it already exists. We will reallocate later.
      this->Normals->Delete();
      this->Normals = NULL;
    }

    vtkPoints *points = this->PolyLine->GetPoints();
    const vtkIdType nPoints = points->GetNumberOfPoints();
    const vtkIdType nLines = nPoints -1;

    // Allocate an array to store the normals

    this->Normals = vtkDoubleArray::New();
    this->Normals->SetNumberOfComponents(3);
    this->Normals->Allocate(3*nLines);
    this->Normals->SetName("Normals");
    this->Normals->SetNumberOfTuples(nLines);

    // Now interate through all the lines and compute normal of each plane
    // in the polyplane.

    double v1[3], p[3], n[3];

    for (int pIdx = 0; pIdx < nLines; ++pIdx)
    {
      // Compute the plane normal for this segment by taking the cross product
      // of the line direction and the extrusion direction.

      points->GetPoint(pIdx, p);
      points->GetPoint(pIdx+1, v1);

      // The line direction vector
      v1[0] -= p[0];
      v1[1] -= p[1];
      v1[2] -= p[2];

      // 'n' is the computed normal.
      vtkMath::Cross( v1, this->ExtrusionDirection, n );
      vtkMath::Normalize(n);

      // Store the normal in our array.
      this->Normals->SetTuple(pIdx, n);
    }
  }
}

//----------------------------------------------------------------------------
// Evaluate the distance to the poly plane for point x[3].
double vtkPolyPlane::EvaluateFunction(double x[3])
{
  // Sanity check
  if (!this->PolyLine || !this->PolyLine->GetPoints())
  {
    return 0;
  }

  double xFlat[3] = {x[0], x[1], 0.0};

  // No error checking, for speed... We will assume that we have a polyline
  // and that it has at least 2 points.
  // traverse the list of points in the polyline.
  vtkPoints *points = this->PolyLine->GetPoints();

  const vtkIdType nPoints = points->GetNumberOfPoints();
  const vtkIdType nLines = nPoints -1;

  // At least 2 points needed to define a polyplane.
  if (nLines < 1)
  {
    return 0;
  }

  // compute normals
  this->ComputeNormals();


  double p1[3], p2[3], t, closest[3];
  double minDistance2 = VTK_DOUBLE_MAX, distance2, signedDistance = VTK_DOUBLE_MAX, sign = 1;

  // Iterate through all the lines.

  for (int pIdx = 0; pIdx < nLines; ++pIdx)
  {

    // Get the end points of this line segment in the polyline
    points->GetPoint(pIdx, p1);
    points->GetPoint(pIdx+1, p2);

    // Flatten it.
    p1[2] = 0;
    p2[2] = 0;

    // Compute distance-squared to finite line. Store the closest point.
    distance2 = vtkLine::DistanceToLine( xFlat, p1, p2, t, closest );

    // if the closest point on the line is on the segment
    if (t >= 0 && t <= 1)
    {
      // if this is the minimum distance found, use that distance
      // and record whether it was right of or left of the line
      if (distance2 < minDistance2)
      {
        minDistance2 = distance2;
        sign = leftOf(p1,p2,xFlat) ? 1 : -1;
      }
    }
    // if the closest point on the line is before the segment starts
    else if (t < 0)
    {
      // compute the distance to the first point on the segment
      distance2 = vtkMath::Distance2BetweenPoints(p1,xFlat);
      // if that is the closest distance use that distance
      if (distance2 < minDistance2)
      {
        minDistance2 = distance2;
        // if this is not the first segment
        if (pIdx > 0)
        {
          double p0[3];
          points->GetPoint(pIdx-1,p0);
          bool leftOf01 = leftOf(p0,p1,xFlat);
          bool leftOf12 = leftOf(p1,p2,xFlat);
          // if the segment before turned left to make this one,
          // the point is to the left only if it is left of both of them
          if (leftOf(p0,p1,p2))
          {
            sign = (leftOf01 && leftOf12) ? 1 : -1;
          }
          // if the segment before turned right to make this one,
          // the point is to the left if it is left of either one
          else
          {
            sign = (leftOf01 || leftOf12) ? 1 : -1;
          }
        }
        // if this is the first segment record if the point is right of
        // or left of the line
        else
        {
          sign = leftOf(p1,p2,xFlat) ? 1 : -1;
        }
      }
    }
    // if the closest point is after the segment ends
    else if (t > 1)
    {
      // compute the distance to the last point on the segment
      distance2 = vtkMath::Distance2BetweenPoints(p2,xFlat);
      // if that is closer than the minimum distance
      if (distance2 < minDistance2)
      {
        // record the distance and
        minDistance2 = distance2;
        // if this is not the last segment
        if (pIdx + 1 < nLines)
        {
          double p3[3];
          points->GetPoint(pIdx+2,p3);
          bool leftOf12 = leftOf(p1,p2,xFlat);
          bool leftOf23 = leftOf(p2,p3,xFlat);
          // if the turn at the end of this segment is a left turn
          // the point is left of the polyline only if left of both
          if (leftOf(p1,p2,p3))
          {
            sign = (leftOf12 && leftOf23) ? 1 : -1;
          }
          // if the turn is a right turn, the point is left of the
          // polyline if it is left of either
          else
          {
            sign = (leftOf12 || leftOf23) ? 1 : -1;
          }
        }
        // if this is the last segment record if the point
        // is left of the segment
        else
        {
          sign = leftOf(p1,p2,xFlat) ? 1 : -1;
        }
      }
    }
  }
  // compute the signed distance to the point
  // negative if it is right of the polyline
  signedDistance = sqrt(minDistance2) * sign;

  return signedDistance;
}

//----------------------------------------------------------------------------
// Evaluate function gradient at point x[3]. We simply return [0,1,0], ie the
// Y Axis.
void vtkPolyPlane::EvaluateGradient(double vtkNotUsed(x)[3], double n[3])
{
  n[0] = 0;
  n[1] = 1;
  n[2] = 0;
}

//----------------------------------------------------------------------------
void vtkPolyPlane::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ExtrusionDirection: (" << this->ExtrusionDirection[0] << ", "
    << this->ExtrusionDirection[1] << ", " << this->ExtrusionDirection[2] << ")\n";

  os << indent << "PolyLine: " << this->PolyLine << "\n";
  if (this->PolyLine)
  {
    this->PolyLine->PrintSelf(os,indent.GetNextIndent());
  }

  os << indent << "Normals: " << this->Normals << "\n";
  if (this->Normals)
  {
    this->Normals->PrintSelf(os,indent.GetNextIndent());
  }
}
