/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoAdaptiveArcs.cxx

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
#include "vtkGeoAdaptiveArcs.h"

#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCoordinate.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkGeoMath.h"
#include "vtkGlobeSource.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkRenderWindow.h"
#include "vtkRendererCollection.h"
#include "vtkTimerLog.h"

#include <vtksys/stl/map>
using vtksys_stl::map;

vtkStandardNewMacro(vtkGeoAdaptiveArcs);

//-------------------------------------------------------------------------
vtkGeoAdaptiveArcs::vtkGeoAdaptiveArcs()
{
  this->GlobeRadius = vtkGeoMath::EarthRadiusMeters();
  this->Renderer = 0;
  this->MaximumPixelSeparation = 10.0;
  this->MinimumPixelSeparation = 1.0;
  this->LastInput = 0;
  this->LastInputMTime = 0;
  this->InputLatitude = vtkDoubleArray::New();
  this->InputLongitude = vtkDoubleArray::New();
}

//-------------------------------------------------------------------------
vtkGeoAdaptiveArcs::~vtkGeoAdaptiveArcs()
{
  this->InputLatitude->Delete();
  this->InputLongitude->Delete();
}

//-------------------------------------------------------------------------
int vtkGeoAdaptiveArcs::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  if (!this->Renderer)
    {
    vtkErrorMacro("Renderer cannot be null.");
    return 0;
    }
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPolyData *input = vtkPolyData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // if the input has changed, compute helper arrays
  if (input != this->LastInput ||
      input->GetMTime() > this->LastInputMTime)
    {
    this->InputLatitude->Initialize();
    this->InputLongitude->Initialize();
    vtkPoints* points = input->GetPoints();
    double curPtLL[2];
    double curPoint[3];
    for (vtkIdType i = 0; i < input->GetNumberOfPoints(); ++i)
      {
      points->GetPoint(i, curPoint);
      vtkGlobeSource::ComputeLatitudeLongitude(
        curPoint, curPtLL[0], curPtLL[1]);
      this->InputLongitude->InsertNextValue(curPtLL[0]);
      this->InputLatitude->InsertNextValue(curPtLL[1]);
      }
    this->LastInput = input;
    this->LastInputMTime = input->GetMTime();
    }

  // Traverse input lines, adding a circle for each line segment.
  int *renSize = this->Renderer->GetSize();
  // Maximum distance from center of renderer.
  double maxDist = (renSize[0] > renSize[1]) ? 1.1*renSize[0]/2.0 : 1.1*renSize[1]/2.0;
  vtkCellArray* lines = input->GetLines();
  vtkCellArray* newLines = vtkCellArray::New();
  vtkPoints* points = input->GetPoints();
  float* pointsPtr = static_cast<float*>(points->GetVoidPointer(0));
  vtkPoints* newPoints = vtkPoints::New();
  double viewAngle = this->Renderer->GetActiveCamera()->GetViewAngle();
  double cameraPos[3];
  this->Renderer->GetActiveCamera()->GetPosition(cameraPos);
  double cameraDir[3];
  this->Renderer->GetActiveCamera()->GetDirectionOfProjection(cameraDir);
  lines->InitTraversal();
  for (vtkIdType i = 0; i < lines->GetNumberOfCells(); i++)
    {
    vtkIdType npts=0; // to remove warning
    vtkIdType* pts=0; // to remove warning
    lines->GetNextCell(npts, pts);

    bool lastPointOffScreen = false;
    bool lastPointTooClose = false;
#if defined(VTK_AGGRESSIVE_ARCS)
    bool lastPointOnOtherSide = false;
#endif
    double curPoint[3];
    double lastPtLL[2] = {0.0, 0.0};
    double curPtLL[2];
    double lastVec[3] = {0.0, 0.0, 0.0};
    double curVec[3];
    curPoint[0] = pointsPtr[3*pts[0]+0];
    curPoint[1] = pointsPtr[3*pts[0]+1];
    curPoint[2] = pointsPtr[3*pts[0]+2];
    curPtLL[0] = this->InputLongitude->GetValue(pts[0]);
    curPtLL[1] = this->InputLatitude->GetValue(pts[0]);
    double curVecSize = 0;
    for (int c = 0; c < 3; ++c)
      {
      curVec[c] = curPoint[c] - cameraPos[c];
      curVecSize += curVec[c]*curVec[c];
      }
    curVecSize = sqrt(curVecSize);
    curVec[0] /= curVecSize;
    curVec[1] /= curVecSize;
    curVec[2] /= curVecSize;

    for (vtkIdType p = 1; p < npts; ++p)
      {
      // Advance the point unless the last point was too close.
      if (!lastPointTooClose)
        {
        for (int c = 0; c < 3; ++c)
          {
//          lastPoint[c] = curPoint[c];
          lastVec[c] = curVec[c];
          }
        lastPtLL[0] = curPtLL[0];
        lastPtLL[1] = curPtLL[1];
        }
#if defined(VTK_AGGRESSIVE_ARCS)
      // If this code is uncommented, then
      // Be aggressive ... skip several points if the last
      // one was offscreen or on the other side of the globe.
      if (lastPointOffScreen || lastPointOnOtherSide)
        {
        p += 5;
        if (p >= npts)
          {
          p = npts - 1;
          }
        }
#endif
      curPoint[0] = pointsPtr[3*pts[p]+0];
      curPoint[1] = pointsPtr[3*pts[p]+1];
      curPoint[2] = pointsPtr[3*pts[p]+2];
      curPtLL[0] = this->InputLongitude->GetValue(pts[p]);
      curPtLL[1] = this->InputLatitude->GetValue(pts[p]);
      curVecSize = 0;
      for (int c = 0; c < 3; ++c)
        {
        curVec[c] = curPoint[c] - cameraPos[c];
        curVecSize += curVec[c]*curVec[c];
        }
      curVecSize = sqrt(curVecSize);
      curVec[0] /= curVecSize;
      curVec[1] /= curVecSize;
      curVec[2] /= curVecSize;

      // Clear flags
#if defined(VTK_AGGRESSIVE_ARCS)
      lastPointOnOtherSide = false;
#endif
      lastPointOffScreen = false;
      lastPointTooClose = false;

      // Don't draw lines off the current screen.
      double distFromCenterApprox =
        vtkMath::DegreesFromRadians( acos( curVec[0] * cameraDir[0] +
                                           curVec[1] * cameraDir[1] +
                                           curVec[2] * cameraDir[2] ) )
        / viewAngle * renSize[1];
      if (distFromCenterApprox > maxDist)
        {
        // If both last point and this point are offscreen, skip
        // drawing the line
        if (lastPointOffScreen == true)
          {
          continue;
          }
        lastPointOffScreen = true;
        }

      // Don't draw lines on the other side of the world.
      //if (vtkMath::Dot(curPoint, cameraPos) < 0)
      if (curPoint[0]*cameraPos[0]+
          curPoint[1]*cameraPos[1]+
          curPoint[2]*cameraPos[2] < 0)
        {
#if defined(VTK_AGGRESSIVE_ARCS)
        lastPointOnOtherSide = true;
#endif
        continue;
        }

      double distApprox =
        vtkMath::DegreesFromRadians( acos( lastVec[0] * curVec[0] +
                                           lastVec[1] * curVec[1] +
                                           lastVec[2] * curVec[2] ) )
        / viewAngle * renSize[1];

      // If the points are too close, skip over it to the next point.
      if (distApprox < this->MinimumPixelSeparation)
        {
        lastPointTooClose = true;
        continue;
        }

      // Calculate the number of subdivisions.
      vtkIdType numDivisions = static_cast<vtkIdType>(distApprox / this->MaximumPixelSeparation + 0.5) + 1;
      if (numDivisions < 2)
        {
        numDivisions = 2;
        }

      // Create the new cell
      newLines->InsertNextCell(numDivisions);

      for (vtkIdType s = 0; s < numDivisions; ++s)
        {
        // Interpolate in lat-long.
        double interpPtLL[2];
        double frac = static_cast<double>(s) / (numDivisions - 1);
        for (int c = 0; c < 2; ++c)
          {
          interpPtLL[c] = frac*curPtLL[c] + (1.0 - frac)*lastPtLL[c];
          }
        // Convert lat-long to world;
        double interpPt[3];
        vtkGlobeSource::ComputeGlobePoint(interpPtLL[0], interpPtLL[1], this->GlobeRadius, interpPt);
        vtkIdType newPt = newPoints->InsertNextPoint(interpPt);
        newLines->InsertCellPoint(newPt);
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

//-------------------------------------------------------------------------
void vtkGeoAdaptiveArcs::SetRenderer(vtkRenderer *ren)
{
  // Do not reference count this, it will cause a loop.
  this->Renderer = ren;
}

//-------------------------------------------------------------------------
unsigned long vtkGeoAdaptiveArcs::GetMTime()
{
  unsigned long retMTime = this->Superclass::GetMTime();
  if ( this->Renderer )
    {
    unsigned long tmpTime = this->Renderer->GetMTime();
    if ( tmpTime > retMTime )
      {
      retMTime = tmpTime;
      }
    vtkCamera* cam = this->Renderer->GetActiveCamera();
    if ( cam )
      {
      tmpTime = cam->GetMTime();
      if ( tmpTime > retMTime )
       retMTime = tmpTime;
      }
    }
  return retMTime;
}

//-------------------------------------------------------------------------
void vtkGeoAdaptiveArcs::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "GlobeRadius: " << this->GlobeRadius << endl;
  os << indent << "MinumumPixelSeparation: " << this->MinimumPixelSeparation << endl;
  os << indent << "MaximumPixelSeparation: " << this->MaximumPixelSeparation << endl;
  os << indent << "Renderer: " << (this->Renderer ? "" : "(null)") << endl;
  if (this->Renderer)
    {
    this->Renderer->PrintSelf(os, indent.GetNextIndent());
    }
}

