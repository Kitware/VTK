/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoArcs.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
#include "vtkGeoArcs.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkGeoMath.h"
#include "vtkGlobeSource.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

#include <vtksys/stl/map>
using vtksys_stl::map;

vtkStandardNewMacro(vtkGeoArcs);

vtkGeoArcs::vtkGeoArcs()
{
  this->GlobeRadius = vtkGeoMath::EarthRadiusMeters();
  this->ExplodeFactor = 0.2;
  this->NumberOfSubdivisions = 20;
}

int vtkGeoArcs::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPolyData *input = vtkPolyData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Prepare to copy cell data
  output->GetCellData()->CopyAllocate(input->GetCellData());

  // Traverse input lines, adding a circle for each line segment.
  vtkCellArray* lines = input->GetLines();
  vtkCellArray* newLines = vtkCellArray::New();
  vtkPoints* newPoints = vtkPoints::New();
  newPoints->DeepCopy(input->GetPoints());
  lines->InitTraversal();
  for (vtkIdType i = 0; i < lines->GetNumberOfCells(); i++)
    {
      vtkIdType npts=0; // to remove warning
    vtkIdType* pts=0; // to remove warning
    lines->GetNextCell(npts, pts);

    double lastPoint[3];
    newPoints->GetPoint(pts[0], lastPoint);

    for (vtkIdType p = 1; p < npts; ++p)
      {
      // Create the new cell
      vtkIdType cellId = newLines->InsertNextCell(this->NumberOfSubdivisions);
      output->GetCellData()->CopyData(input->GetCellData(), i, cellId);

      double curPoint[3];
      newPoints->GetPoint(pts[p], curPoint);

      // Find w, a unit vector pointing from the center of the
      // earth directly inbetween the two endpoints.
      double w[3];
      for (int c = 0; c < 3; ++c)
        {
        w[c] = (lastPoint[c] + curPoint[c])/2.0;
        }
      vtkMath::Normalize(w);

      // The center of the circle used to draw the arc is a
      // point along the vector w scaled by the explode factor.
      double center[3];
      for (int c = 0; c < 3; ++c)
        {
        center[c] = this->ExplodeFactor * this->GlobeRadius * w[c];
        }

      // The vectors u and x are unit vectors pointing from the
      // center of the circle to the two endpoints of the arc,
      // lastPoint and curPoint, respectively.
      double u[3], x[3];
      for (int c = 0; c < 3; ++c)
        {
        u[c] = lastPoint[c] - center[c];
        x[c] = curPoint[c] - center[c];
        }
      double radius = vtkMath::Norm(u);
      vtkMath::Normalize(u);
      vtkMath::Normalize(x);

      // Find the angle that the arc spans.
      double theta = acos(vtkMath::Dot(u, x));

      // If the vectors u, x point toward the center of the earth, take
      // the larger angle between the vectors.
      // We determine whether u points toward the center of the earth
      // by checking whether the dot product of u and w is negative.
      if (vtkMath::Dot(w, u) < 0)
        {
        theta = 2.0*vtkMath::Pi() - theta;
        }

      // We need two perpendicular vectors on the plane of the circle
      // in order to draw the circle.  First we calculate n, a vector
      // normal to the circle, by crossing u and w.  Next, we cross
      // n and u in order to get a vector v in the plane of the circle
      // that is perpendicular to u.
      double n[3];
      vtkMath::Cross(u, w, n);
      vtkMath::Normalize(n);
      double v[3];
      vtkMath::Cross(n, u, v);
      vtkMath::Normalize(v);

      // Use the general equation for a circle in three dimensions
      // to draw an arc from the last point to the current point.
      for (int s = 0; s < this->NumberOfSubdivisions; ++s)
        {
        double angle = s * theta / (this->NumberOfSubdivisions - 1.0);
        double circlePt[3];
        for (int c = 0; c < 3; ++c)
          {
          circlePt[c] = center[c] + radius*cos(angle)*u[c] + radius*sin(angle)*v[c];
          }
        vtkIdType newPt = newPoints->InsertNextPoint(circlePt);
        newLines->InsertCellPoint(newPt);
        }

      for (int c = 0; c < 3; ++c)
        {
        lastPoint[c] = curPoint[c];
        }
      }
    }

  // Send the data to output.
  output->SetLines(newLines);
  output->SetPoints(newPoints);

  // Clean up.
  newLines->Delete();
  newPoints->Delete();

  return 1;
}

void vtkGeoArcs::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "GlobeRadius: " << this->GlobeRadius << endl;
  os << indent << "ExplodeFactor: " << this->ExplodeFactor << endl;
  os << indent << "NumberOfSubdivisions: " << this->NumberOfSubdivisions << endl;
}
