/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoAssignCoordinates.cxx

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

#include "vtkGeoAssignCoordinates.h"

#include "vtkAbstractTransform.h"
#include "vtkGeoMath.h"
#include "vtkGlobeSource.h"
#include "vtkGraph.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"

vtkCxxSetObjectMacro(vtkGeoAssignCoordinates, Transform, vtkAbstractTransform);
vtkStandardNewMacro(vtkGeoAssignCoordinates);

vtkGeoAssignCoordinates::vtkGeoAssignCoordinates()
{
  this->LongitudeArrayName = 0;
  this->LatitudeArrayName = 0;
  this->CoordinatesInArrays = true;
  this->Transform = 0;

  this->GlobeRadius = vtkGeoMath::EarthRadiusMeters();
}

vtkGeoAssignCoordinates::~vtkGeoAssignCoordinates()
{
  if (this->Transform)
    {
    this->Transform->Delete();
    }
  if(this->LongitudeArrayName!=0)
    {
    delete[] this->LongitudeArrayName;
    }
  if(this->LatitudeArrayName!=0)
    {
    delete[] this->LatitudeArrayName;
    }
}


int vtkGeoAssignCoordinates::RequestData(vtkInformation *vtkNotUsed(request),
                            vtkInformationVector **inputVector,
                            vtkInformationVector *outputVector)
{

  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataObject *input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkDataObject *output = outInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkGraph *graphInput = vtkGraph::SafeDownCast(input);
  vtkGraph *graphOutput = vtkGraph::SafeDownCast(output);
  vtkPointSet *psInput = vtkPointSet::SafeDownCast(input);
  vtkPointSet *psOutput = vtkPointSet::SafeDownCast(output);
  
  // Do a shallow copy of the input to the output
  // and then create new points on the output
  output->ShallowCopy(input);
  vtkPoints *newPoints = vtkPoints::New();
  vtkPoints *oldPoints = 0;
  vtkDataSetAttributes *pd = 0;
  vtkIdType numPoints = 0;
  if (graphInput)
    {
    oldPoints = graphInput->GetPoints();
    newPoints->DeepCopy(oldPoints);
    graphOutput->SetPoints(newPoints);
    pd = graphOutput->GetVertexData();
    numPoints = graphInput->GetNumberOfVertices();
    }
  else
    {
    oldPoints = psInput->GetPoints();
    newPoints->DeepCopy(oldPoints);
    psOutput->SetPoints(newPoints);
    pd = psOutput->GetPointData();
    numPoints = psInput->GetNumberOfPoints();
    }
  newPoints->Delete();
    
  // If there are no points in the input, we're done!
  if (numPoints == 0) 
    {
    return 1;
    }
  
  vtkDataArray* latitudeArray = 0;
  vtkDataArray* longitudeArray = 0;
  if (this->CoordinatesInArrays)
    {
    // I need a latitude array
    if (!this->LatitudeArrayName || strlen(this->LatitudeArrayName) == 0)
      {
      vtkErrorMacro("No latitude array defined.");
      return 0;
      }
    
    // I need a longitude array
    if (!this->LongitudeArrayName || strlen(this->LongitudeArrayName) == 0)
      {  // If on, uses LatitudeArrayName and LongitudeArrayName to
      // move values in data arrays into the points of the data set.
      // Turn off if the lattitude and longitude are already in
      // the points.
  
      vtkErrorMacro("No longitude array defined.");
      return 0;
      }
    
    // Okay now check for arrays
    latitudeArray = pd->GetArray(this->LatitudeArrayName);
    
    // Does the latitude array exist at all?  
    if (this->CoordinatesInArrays && latitudeArray == NULL)
      {
      vtkErrorMacro("Could not find array named " << this->LatitudeArrayName);
      return 0;
      }
      
    // Longitude coordinate array
    longitudeArray = pd->GetArray(this->LongitudeArrayName);
  
    // Does the array exist at all?  
    if (this->CoordinatesInArrays && longitudeArray == NULL)
      {
      vtkErrorMacro("Could not find array named " << this->LongitudeArrayName);
      return 0;
      }
    }
    
  // Convert the points to global coordinates
  for (int i = 0; i < numPoints; i++)
    {
    double theta, phi;
    if (this->CoordinatesInArrays)
      {
      theta = longitudeArray->GetTuple1(i);
      phi = latitudeArray->GetTuple1(i);
      }
    else
      {
      double a[3];
      oldPoints->GetPoint(i, a);
      theta = a[0];
      phi = a[1];
      }

    // Clamp to lat/long bounds
    theta = (theta >  180.0) ?  180.0 : theta;
    theta = (theta < -180.0) ? -180.0 : theta;
    phi = (phi >  90.0) ?  90.0 : phi;
    phi = (phi < -90.0) ? -90.0 : phi;

    double x[3];
    if (this->Transform)
      {
      double in[] = {theta, phi, 0.0};
      this->Transform->TransformPoint(in, x);
      }
    else
      {
      vtkGlobeSource::ComputeGlobePoint(theta, phi, this->GlobeRadius, x);
      }
    newPoints->SetPoint(i, x[0], x[1], x[2]);
    }
    
  return 1;
} 

int vtkGeoAssignCoordinates::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  // This algorithm may accept a vtkPointSet or vtkGraph.
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
  return 1;  
}

void vtkGeoAssignCoordinates::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "LatitudeArrayName: " 
     << (this->LatitudeArrayName ? this->LatitudeArrayName : "(none)") << endl;
  os << indent << "LongitudeArrayName: " 
     << (this->LongitudeArrayName ? this->LongitudeArrayName : "(none)") << endl;     
  os << indent << "GlobeRadius: " << this->GlobeRadius << endl;
  os << indent << "CoordinatesInArrays: " << (this->CoordinatesInArrays ? "on" : "off") << endl;
  os << indent << "Transform: " << (this->Transform ? "" : "(none)") << endl;
  if (this->Transform)
    {
    this->Transform->PrintSelf(os, indent.GetNextIndent());
    }
}
