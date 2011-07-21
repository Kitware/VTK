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
unsigned long vtkPolyPlane::GetMTime()
{
  unsigned long mTime = this->Superclass::GetMTime();

  if (this->PolyLine)
    {
    unsigned long p1Time;
    p1Time = this->PolyLine->GetMTime();
    mTime = ( p1Time > mTime ? p1Time : mTime );
    }

  return mTime;
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


  double p1[3], p2[3], t, closest[3], normal[3];
  double minDistance2 = VTK_DOUBLE_MAX, distance2, signedDistance = VTK_DOUBLE_MAX;

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

    // If this is the closest point, compute the signed distance
    // Make sure that this point will intersect, ie    t E [0,1]
    if (distance2 < minDistance2 && t >= 0 && t <= 1)
      {
      minDistance2 = distance2;

      // Use the Y Axis to determine the sign by checking the y coordinate of
      // the closest point w.r.t the point xFlat. This is arbitrary, but we
      // need some reference frame to compute smooth signed scalars.

      this->Normals->GetTuple(pIdx, normal);
      const double sign =
        ((closest[0] - xFlat[0])*normal[0] +
         (closest[1] - xFlat[1])*normal[1]) > 0 ? 1 : -1;

      signedDistance = sqrt(minDistance2) * sign;

      this->ClosestPlaneIdx = pIdx;
      }
    }

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
