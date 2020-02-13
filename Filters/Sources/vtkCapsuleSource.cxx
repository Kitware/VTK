/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCapsuleSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.

  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*=========================================================================

  Program: Bender
  Copyright (c) Kitware Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0.txt

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

=========================================================================*/

#include "vtkCapsuleSource.h"

#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <cmath>

vtkStandardNewMacro(vtkCapsuleSource);

//----------------------------------------------------------------------------
vtkCapsuleSource::vtkCapsuleSource(int res)
{
  res = res < 8 ? 8 : res;
  this->Radius = 0.5;
  this->Center[0] = 0.0;
  this->Center[1] = 0.0;
  this->Center[2] = 0.0;
  this->ThetaResolution = res;
  this->PhiResolution = res;
  this->LatLongTessellation = 0;
  this->CylinderLength = 1.0;
  this->OutputPointsPrecision = vtkAlgorithm::DEFAULT_PRECISION;
  this->SetNumberOfInputPorts(0);
}

namespace
{
//----------------------------------------------------------------------------
void InsertPole(
  vtkPoints* points, vtkFloatArray* normals, double center[3], double radius, double halfHeight)
{
  double x[3];
  x[0] = center[0];
  x[1] = center[1] + halfHeight;
  x[2] = center[2] + radius;
  points->InsertNextPoint(x);
  x[0] = x[1] = 0.0;
  x[2] = radius > 0 ? 1.0 : -1.0;
  normals->InsertNextTuple(x);
}

//----------------------------------------------------------------------------
void FillHalfSphere(vtkPoints* points, vtkFloatArray* normals, double thetaResolution,
  double phiResolution, double startAngle, double sign, double center[3], double radius,
  double halfHeight)
{
  double n[3], x[3], norm;
  const double deltaTheta = vtkMath::Pi() / (thetaResolution - 1);
  const double deltaPhi = vtkMath::Pi() / (phiResolution - 1);
  for (int i = 0; i < thetaResolution; ++i)
  {
    const double theta = startAngle + sign * i * deltaTheta;
    for (int j = 1; j < phiResolution - 1; ++j)
    {
      const double phi = j * deltaPhi;
      const double r = radius * std::sin(phi);
      n[0] = r * std::cos(theta);
      n[1] = r * std::sin(theta);
      n[2] = radius * std::cos(phi);
      x[0] = n[0] + center[0];
      x[1] = n[1] + center[1] + halfHeight;
      x[2] = n[2] + center[2];
      points->InsertNextPoint(x);
      if ((norm = vtkMath::Norm(n)) == 0.0)
      {
        norm = 1.0;
      }
      n[0] /= norm;
      n[1] /= norm;
      n[2] /= norm;
      normals->InsertNextTuple(n);
    }
  }
}

//----------------------------------------------------------------------------
void ConnectCylinderSide(vtkCellArray* faces, int minusPoleId, int plusPoleId, int clockwise,
  int increment, int quadrangle)
{
  vtkIdType pts[4];
  for (int i = 0; i < increment; ++i)
  {
    pts[0] = minusPoleId + clockwise * i;
    pts[1] = plusPoleId + clockwise * i;

    if (quadrangle != 0)
    {
      pts[2] = pts[0] + clockwise;
      faces->InsertNextCell(3, pts);

      pts[0] = pts[2];
      pts[2] = pts[1] + clockwise;
      faces->InsertNextCell(3, pts);
    }
    else
    {
      pts[2] = pts[1] + clockwise;
      pts[3] = pts[0] + clockwise;
      faces->InsertNextCell(4, pts);
    }
  }
}
}

//----------------------------------------------------------------------------
int vtkCapsuleSource::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPoints* newPoints;
  vtkFloatArray* newNormals;
  vtkCellArray* newPolys;
  // Set number of points.
  const int halfSphereNumPts = (this->PhiResolution - 2) * this->ThetaResolution + 2;
  const int numPts = halfSphereNumPts * 2;
  // Set number of faces.
  const int halfSpheresNumsPolys =
    (this->ThetaResolution - 1) * 2 + (this->PhiResolution - 3) * (this->ThetaResolution - 1) * 2;
  const int cylinderNumPolys = (this->PhiResolution - 1) * 4;
  const int numPolys = halfSpheresNumsPolys * 2 + cylinderNumPolys;
  // Allocate.
  newPoints = vtkPoints::New();
  if (this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
  {
    newPoints->SetDataType(VTK_DOUBLE);
  }
  else
  {
    newPoints->SetDataType(VTK_FLOAT);
  }
  newPoints->Allocate(numPts);
  newNormals = vtkFloatArray::New();
  newNormals->SetNumberOfComponents(3);
  newNormals->Allocate(numPts * 3);
  newNormals->SetName("Normals");
  newPolys = vtkCellArray::New();
  newPolys->Allocate(newPolys->EstimateSize(numPolys, 3));
  // Create half sphere 1, plus side.
  const double halfHeight = this->CylinderLength * 0.5;
  // North pole.
  InsertPole(newPoints, newNormals, this->Center, this->Radius, halfHeight);
  this->UpdateProgress(0.05);
  // Create intermediate points.
  FillHalfSphere(newPoints, newNormals, this->ThetaResolution, this->PhiResolution, 0.0, 1.0,
    this->Center, this->Radius, halfHeight);
  this->UpdateProgress(0.255);
  // South pole.
  InsertPole(newPoints, newNormals, this->Center, -1 * this->Radius, halfHeight);
  this->UpdateProgress(0.3); // First half sphere done.
  // Create half sphere 2, minus side.
  // North pole.
  InsertPole(newPoints, newNormals, this->Center, this->Radius, -1 * halfHeight);
  this->UpdateProgress(0.305);
  // Create intermediate points.
  FillHalfSphere(newPoints, newNormals, this->ThetaResolution, this->PhiResolution,
    vtkMath::Pi() * 2.0, -1, this->Center, this->Radius, -1.0 * halfHeight);
  this->UpdateProgress(0.555);
  // South pole.
  InsertPole(newPoints, newNormals, this->Center, -1 * this->Radius, -1 * halfHeight);
  this->UpdateProgress(0.6); // Second half sphere done.
  // Generate mesh connectivity.
  vtkIdType pts[4];
  // Increment represent how many ids have passed every time we change by one delta theta.
  const int increment = this->PhiResolution - 2;
  // Ids of the poles.
  const int northPoleMinusId = 0;
  const int southPoleMinusId = halfSphereNumPts - 1;
  const int northPolePlusId = halfSphereNumPts;
  const int southPolePlusId = numPts - 1;
  // First half sphere.
  // Connect the minus side half sphere north pole.
  for (int i = 0; i < this->ThetaResolution - 1; ++i)
  {
    pts[0] = northPoleMinusId;
    pts[1] = (i * increment) + 1;
    pts[2] = pts[1] + increment;
    newPolys->InsertNextCell(3, pts);
  }
  this->UpdateProgress(0.605);
  // South pole connectivity.
  for (int i = 1; i < this->ThetaResolution; ++i)
  {
    pts[0] = i * increment;
    pts[1] = southPoleMinusId;
    pts[2] = pts[0] + increment;
    newPolys->InsertNextCell(3, pts);
  }
  this->UpdateProgress(0.75); // First half-sphere done.
  // Second half sphere.
  // North pole connectivity.
  for (int i = 0; i < this->ThetaResolution - 1; ++i)
  {
    pts[2] = northPolePlusId;
    pts[1] = (i * increment) + 1 + northPolePlusId;
    pts[0] = pts[1] + increment;
    newPolys->InsertNextCell(3, pts);
  }
  this->UpdateProgress(0.755);
  // South pole connectivity.
  for (int i = 1; i < this->ThetaResolution; ++i)
  {
    pts[0] = northPolePlusId + i * increment;
    pts[2] = southPolePlusId;
    pts[1] = pts[0] + increment;
    newPolys->InsertNextCell(3, pts);
  }
  this->UpdateProgress(0.9);
  // Both half sphere at the same time, connectivity of the band.
  vtkIdType ptsOtherSide[4];
  for (int i = 1; i < northPolePlusId - 1 - increment; i += increment)
  {
    for (int j = 0; j < increment - 1; ++j)
    {
      pts[0] = j + i;
      pts[1] = j + i + 1;
      pts[2] = j + i + increment + 1;
      ptsOtherSide[0] = pts[0] + northPolePlusId;
      ptsOtherSide[1] = pts[2] + northPolePlusId;
      ptsOtherSide[2] = pts[1] + northPolePlusId;
      if (!this->LatLongTessellation)
      {
        newPolys->InsertNextCell(3, pts);
        newPolys->InsertNextCell(3, ptsOtherSide);
        pts[1] = pts[2];
        pts[2] = j + i + increment;
        ptsOtherSide[1] = pts[2] + northPolePlusId;
        ptsOtherSide[2] = pts[1] + northPolePlusId;
        newPolys->InsertNextCell(3, pts);
        newPolys->InsertNextCell(3, ptsOtherSide);
      }
      else
      {
        pts[3] = j + i + increment;
        ptsOtherSide[1] = pts[3] + northPolePlusId;
        ptsOtherSide[3] = ptsOtherSide[2];
        ptsOtherSide[2] = pts[2] + northPolePlusId;
        newPolys->InsertNextCell(4, pts);
        newPolys->InsertNextCell(4, ptsOtherSide);
      }
    }
  }
  // Cylinder pole connectivity.
  // First side.
  ConnectCylinderSide(
    newPolys, northPoleMinusId, northPolePlusId, 1, increment, this->LatLongTessellation);
  // Second side.
  ConnectCylinderSide(
    newPolys, southPoleMinusId, southPolePlusId, -1, increment, this->LatLongTessellation);
  this->UpdateProgress(0.99);
  // Weird south pole minus face case.
  pts[0] = northPoleMinusId + increment;
  pts[1] = northPolePlusId + increment;
  if (!this->LatLongTessellation)
  {
    pts[2] = southPoleMinusId;
    newPolys->InsertNextCell(3, pts);
    pts[0] = pts[1];
    pts[1] = southPolePlusId;
    newPolys->InsertNextCell(3, pts);
  }
  else
  {
    pts[2] = southPolePlusId;
    pts[3] = southPoleMinusId;
    newPolys->InsertNextCell(4, pts);
  }
  // Weird north pole 2 face case.
  pts[0] = southPoleMinusId - increment;
  pts[1] = southPolePlusId - increment;
  if (!this->LatLongTessellation)
  {
    pts[2] = northPoleMinusId;
    newPolys->InsertNextCell(3, pts);
    pts[0] = pts[1];
    pts[1] = northPolePlusId;
    newPolys->InsertNextCell(3, pts);
  }
  else
  {
    pts[2] = northPolePlusId;
    pts[3] = northPoleMinusId;
    newPolys->InsertNextCell(4, pts);
  }
  // Update ourselves and release memory.
  newPoints->Squeeze();
  output->SetPoints(newPoints);
  newPoints->Delete();
  newNormals->Squeeze();
  output->GetPointData()->SetNormals(newNormals);
  newNormals->Delete();
  newPolys->Squeeze();
  output->SetPolys(newPolys);
  newPolys->Delete();
  return 1;
}

//----------------------------------------------------------------------------
void vtkCapsuleSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Center: (" << this->Center[0] << ", " << this->Center[1] << ", "
     << this->Center[2] << ")" << std::endl;
  os << indent << "CylinderLength: " << this->CylinderLength << std::endl;
  os << indent << "LatLongTessellation: " << this->LatLongTessellation << std::endl;
  os << indent << "PhiResolution: " << this->PhiResolution << std::endl;
  os << indent << "ThetaResolution: " << this->ThetaResolution << std::endl;
  os << indent << "Radius: " << this->Radius << std::endl;
}

//----------------------------------------------------------------------------
int vtkCapsuleSource::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), -1);
  const double halfLength = this->CylinderLength * 0.5;
  outInfo->Set(vtkStreamingDemandDrivenPipeline::BOUNDS(),
    this->Center[0] - this->Radius - halfLength, this->Center[0] + this->Radius + halfLength,
    this->Center[1] - this->Radius, this->Center[1] + this->Radius, this->Center[2] - this->Radius,
    this->Center[2] + this->Radius);
  return 1;
}
