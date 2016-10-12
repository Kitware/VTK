/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGlobeSource.cxx

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

#include "vtkGlobeSource.h"

#include "vtkCellArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkGeoMath.h"
#include "vtkTimerLog.h"

#include <cmath>

vtkStandardNewMacro(vtkGlobeSource);

  // 0=NE, 1=SE, 2=SW, 3=NW

//----------------------------------------------------------------------------
vtkGlobeSource::vtkGlobeSource()
{
  this->Origin[0] = this->Origin[1] = this->Origin[2] = 0.0;
  this->Radius = vtkGeoMath::EarthRadiusMeters();
  this->AutoCalculateCurtainHeight = true;
  this->CurtainHeight = 1000.0;

  this->LongitudeResolution = 10;
  this->LatitudeResolution = 10;
  this->StartLongitude = 0.0;
  this->EndLongitude = 360.0;
  this->StartLatitude = 0.0;
  this->EndLatitude = 180.0;
  this->QuadrilateralTessellation = 0;

  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
void vtkGlobeSource::ComputeGlobePoint(
  double theta, double phi, double radius, double* x, double* normal)
{
  // Lets keep this conversion code in a single place.
  double tmp = cos( vtkMath::RadiansFromDegrees( phi ) );
  double n0 = -tmp * sin( vtkMath::RadiansFromDegrees( theta ) );
  double n1 = tmp * cos( vtkMath::RadiansFromDegrees( theta ) );
  double n2 = sin( vtkMath::RadiansFromDegrees( phi ) );

  x[0] = n0 * radius;
  x[1] = n1 * radius;
  x[2] = n2 * radius;

  if (normal)
  {
    normal[0] = n0;
    normal[1] = n1;
    normal[2] = n2;
  }
}

//----------------------------------------------------------------------------
void vtkGlobeSource::ComputeLatitudeLongitude(
  double* x, double& theta, double& phi)
{
  double rho = sqrt(x[0]*x[0] + x[1]*x[1] + x[2]*x[2]);
  double S = sqrt(x[0]*x[0] + x[1]*x[1]);
  phi = acos(x[2] / rho);
  if (x[0] >= 0)
  {
    theta = asin(x[1] / S);
  }
  else
  {
    theta = vtkMath::Pi() - asin(x[1] / S);
  }
  phi =   vtkMath::DegreesFromRadians( vtkMath::Pi() / 2.0 - phi );
  theta = vtkMath::DegreesFromRadians( theta - vtkMath::Pi()/2.0 );
}

//----------------------------------------------------------------------------
void vtkGlobeSource::AddPoint(
  double theta, double phi, double radius,
  vtkPoints* newPoints, vtkFloatArray* newNormals,
  vtkFloatArray* newLongitudeArray, vtkFloatArray* newLatitudeArray,
  vtkDoubleArray* newLatLongArray)
{
  double x[3], n[3];

  vtkGlobeSource::ComputeGlobePoint(theta, phi, radius, x, n);

  x[0] -= this->Origin[0];
  x[1] -= this->Origin[1];
  x[2] -= this->Origin[2];

  newPoints->InsertNextPoint(x);
  newNormals->InsertNextTuple(n);

  newLongitudeArray->InsertNextValue(theta);
  newLatitudeArray->InsertNextValue(phi);
  newLatLongArray->InsertNextValue(phi);
  newLatLongArray->InsertNextValue(theta);
}



//----------------------------------------------------------------------------
int vtkGlobeSource::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // I used this to see if the background thread was really
  // operating asynchronously.
  //Sleep(2000);

  // I am going to compute the curtain height based on the level of the
  // terrain patch.
  if(this->AutoCalculateCurtainHeight)
  {
    this->CurtainHeight = (this->EndLongitude-this->StartLongitude)
      * this->Radius / 3600.0;
  }

  // get the ouptut
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  int i, j;
  int numPts, numPolys;
  vtkPoints *newPoints;
  vtkFloatArray *newLongitudeArray;
  vtkFloatArray *newLatitudeArray;
  vtkDoubleArray *newLatLongArray;
  vtkFloatArray *newNormals;
  vtkCellArray *newPolys;
  double phi, theta;
  vtkIdType pts[4];

  // Set things up; allocate memory
  //
  numPts = this->LatitudeResolution * this->LongitudeResolution;
  // creating triangles
  numPolys = (this->LatitudeResolution-1)*(this->LongitudeResolution-1) * 2;

  // Add more for curtains.
  numPts += 2*(this->LatitudeResolution + this->LongitudeResolution);

  newPoints = vtkPoints::New();
  newPoints->Allocate(numPts);
  newNormals = vtkFloatArray::New();
  newNormals->SetNumberOfComponents(3);
  newNormals->Allocate(3*numPts);
  newNormals->SetName("Normals");

  newLongitudeArray = vtkFloatArray::New();
  newLongitudeArray->SetNumberOfComponents(1);
  newLongitudeArray->Allocate(numPts);
  newLongitudeArray->SetName("Longitude");

  newLatitudeArray = vtkFloatArray::New();
  newLatitudeArray->SetNumberOfComponents(1);
  newLatitudeArray->Allocate(numPts);
  newLatitudeArray->SetName("Latitude");

  newLatLongArray = vtkDoubleArray::New();
  newLatLongArray->SetNumberOfComponents(2);
  newLatLongArray->Allocate(2*numPts);
  newLatLongArray->SetName("LatLong");

  newPolys = vtkCellArray::New();
  newPolys->Allocate(newPolys->EstimateSize(numPolys, 3));

  // Create sphere
  //

  double deltaLongitude;
  double deltaLatitude;

  // Check data, determine increments, and convert to radians
  deltaLongitude = (this->EndLongitude - this->StartLongitude)
                 / static_cast<double>(this->LongitudeResolution-1);

  deltaLatitude = (this->EndLatitude - this->StartLatitude)
                 / static_cast<double>(this->LatitudeResolution-1);

  // Create points and point data.
  for (j=0; j<this->LatitudeResolution; j++)
  {
    phi = this->StartLatitude + j*deltaLatitude;
    for (i=0; i < this->LongitudeResolution; i++)
    {
      theta = this->StartLongitude + i*deltaLongitude;
      this->AddPoint(theta, phi, this->Radius,
                     newPoints, newNormals,
                     newLongitudeArray, newLatitudeArray,
                     newLatLongArray);
    }
    this->UpdateProgress(
      0.10 + 0.50*j/static_cast<float>(this->LatitudeResolution));
  }

  // Create the extra points for the curtains.
  for (i=0; i < this->LongitudeResolution; i++)
  {
    theta = this->StartLongitude + i*deltaLongitude;
    phi = this->StartLatitude;
    this->AddPoint(theta, phi, this->Radius-this->CurtainHeight,
                   newPoints, newNormals,
                   newLongitudeArray, newLatitudeArray,
                   newLatLongArray);
  }
  for (i=0; i < this->LongitudeResolution; i++)
  {
    theta = this->StartLongitude + i*deltaLongitude;
    phi = this->EndLatitude;
    this->AddPoint(theta, phi, this->Radius-this->CurtainHeight,
                   newPoints, newNormals,
                   newLongitudeArray, newLatitudeArray,
                   newLatLongArray);
  }
  for (j=0; j < this->LatitudeResolution; j++)
  {
    theta = this->StartLongitude;
    phi = this->StartLatitude + j*deltaLatitude;
    this->AddPoint(theta, phi, this->Radius-this->CurtainHeight,
                   newPoints, newNormals,
                   newLongitudeArray, newLatitudeArray,
                   newLatLongArray);
  }
  for (j=0; j < this->LatitudeResolution; j++)
  {
    theta = this->EndLongitude;
    phi = this->StartLatitude + j*deltaLatitude;
    this->AddPoint(theta, phi, this->Radius-this->CurtainHeight,
                   newPoints, newNormals,
                   newLongitudeArray, newLatitudeArray,
                   newLatLongArray);
  }


  // Generate mesh connectivity
  vtkIdType rowId = 0;
  vtkIdType cornerId;
  for (j=1; j < this->LatitudeResolution; ++j)
  {
    cornerId = rowId;
    for (i=1; i < this->LongitudeResolution; ++i)
    {
      pts[0] = cornerId;
      pts[2] = cornerId + this->LongitudeResolution;
      pts[1] = pts[2] + 1;
      newPolys->InsertNextCell(3, pts);
      pts[2] = pts[1];
      pts[1] = cornerId + 1;
      newPolys->InsertNextCell(3, pts);
      ++cornerId;
    }
    rowId += this->LongitudeResolution;
    this->UpdateProgress(
      0.70 + 0.30*j/static_cast<double>(this->LatitudeResolution));
  }

  // Create curtain quads.
  vtkIdType curtainPointId =
    this->LongitudeResolution * this->LatitudeResolution;
  vtkIdType edgeOffset;
  edgeOffset = 0;
  for (i=1; i < this->LongitudeResolution; ++i)
  {
    pts[0] = edgeOffset + i; // i starts at 1.
    pts[1] = pts[0] - 1;
    pts[2] = curtainPointId;
    pts[3] = curtainPointId + 1;
    newPolys->InsertNextCell(4, pts);
    ++curtainPointId;
  }
  ++curtainPointId; // Skip 2 to the next edge.
  edgeOffset = (this->LongitudeResolution)*(this->LatitudeResolution-1);
  for (i=1; i < this->LongitudeResolution; ++i)
  {
    pts[0] = edgeOffset + i - 1; // i starts at 1
    pts[1] = pts[0] + 1;
    pts[2] = curtainPointId + 1;
    pts[3] = curtainPointId;
    newPolys->InsertNextCell(4, pts);
    ++curtainPointId;
  }
  ++curtainPointId;
  edgeOffset = 0;
  for (j=1; j < this->LatitudeResolution; ++j)
  {
    pts[0] = edgeOffset + j*this->LongitudeResolution;
    pts[1] = pts[0] - this->LongitudeResolution;
    pts[2] = curtainPointId;
    pts[3] = curtainPointId + 1;
    newPolys->InsertNextCell(4, pts);
    ++curtainPointId;
  }
  ++curtainPointId;
  edgeOffset = (this->LongitudeResolution-1);
  for (j=1; j < this->LatitudeResolution; ++j)
  {
    pts[0] = edgeOffset + (j-1)*this->LongitudeResolution;
    pts[1] = pts[0] + this->LongitudeResolution;
    pts[2] = curtainPointId + 1;
    pts[3] = curtainPointId;
    newPolys->InsertNextCell(4, pts);
    ++curtainPointId;
  }


  // Update ourselves and release memeory
  //
  newPoints->Squeeze();
  output->SetPoints(newPoints);
  newPoints->Delete();

  newNormals->Squeeze();
  output->GetPointData()->SetNormals(newNormals);
  newNormals->Delete();

  newLongitudeArray->Squeeze();
  output->GetPointData()->AddArray(newLongitudeArray);
  newLongitudeArray->Delete();

  newLatitudeArray->Squeeze();
  output->GetPointData()->AddArray(newLatitudeArray);
  newLatitudeArray->Delete();

  newLatLongArray->Squeeze();
  output->GetPointData()->AddArray(newLatLongArray);
  newLatLongArray->Delete();

  newPolys->Squeeze();
  output->SetPolys(newPolys);
  newPolys->Delete();

  return 1;
}

//----------------------------------------------------------------------------
void vtkGlobeSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "AutoCalculateCurtainHeight: " <<
    (this->AutoCalculateCurtainHeight ? "ON" : "OFF") << "\n";
  os << indent << "CurtainHeight: " << this->CurtainHeight << "\n";
  os << indent << "Longitude Resolution: " << this->LongitudeResolution << "\n";
  os << indent << "Latitude Resolution: " << this->LatitudeResolution << "\n";
  os << indent << "Longitude Start: " << this->StartLongitude << "\n";
  os << indent << "Latitude Start: " << this->StartLatitude << "\n";
  os << indent << "Longitude End: " << this->EndLongitude << "\n";
  os << indent << "Latitude End: " << this->EndLatitude << "\n";
  os << indent << "Radius: " << this->Radius << "\n";
  os << indent << "Origin: " << this->Origin[0] << ","
                             << this->Origin[1] << ","
                             << this->Origin[2] << "\n";
  os << indent
     << "Quadrilateral Tessellation: "
     << this->QuadrilateralTessellation << "\n";
}
