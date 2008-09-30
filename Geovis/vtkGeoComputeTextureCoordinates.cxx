/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoComputeTextureCoordinates.cxx

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
#include "vtkGeoComputeTextureCoordinates.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkFloatArray.h"

vtkCxxRevisionMacro(vtkGeoComputeTextureCoordinates, "1.2");
vtkStandardNewMacro(vtkGeoComputeTextureCoordinates);


vtkGeoComputeTextureCoordinates::vtkGeoComputeTextureCoordinates()
{
  // Default to covering the whold earth.
  this->ImageLongitudeLatitudeExtent[0] = -180.0;
  this->ImageLongitudeLatitudeExtent[1] = 180.0;
  this->ImageLongitudeLatitudeExtent[2] = -90.0;
  this->ImageLongitudeLatitudeExtent[3] = 90.0;
}

void vtkGeoComputeTextureCoordinates::Execute()
{
  vtkFloatArray *longitudeArray;
  vtkFloatArray *latitudeArray;
  vtkFloatArray *newTCoords;
  vtkPolyData *input = this->GetInput();
  vtkPolyData *output = this->GetOutput();
  
  vtkIdType numPts = input->GetNumberOfPoints();

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );
  
  longitudeArray = vtkFloatArray::SafeDownCast(
    input->GetPointData()->GetArray("Longitude"));
  latitudeArray =  vtkFloatArray::SafeDownCast(
    input->GetPointData()->GetArray("Latitude"));
  if (longitudeArray == 0)
    {
    vtkErrorMacro("No float longitude array to transform.");
    return;
    }
  if (latitudeArray == 0)
    {
    vtkErrorMacro("No float latitude array to transform.");
    return;
    }
  if (longitudeArray->GetNumberOfComponents() != 1 ||
      latitudeArray->GetNumberOfComponents() != 1)
    {
    vtkErrorMacro("Expecting 1 component for longitude and latitude arrays.");
    return;
    }

  //  Allocate texture data
  //
  newTCoords = vtkFloatArray::New();
  newTCoords->SetName("TextureCoordinates");
  newTCoords->SetNumberOfComponents(2);
  newTCoords->SetNumberOfTuples(numPts);

  //  Now can loop over all points, computing texture coordinates.
  float *longitudePtr = longitudeArray->GetPointer(0);
  float *latitudePtr = latitudeArray->GetPointer(0);
  float *tcoordPtr = newTCoords->GetPointer(0);
  
  // In: Longitude 0->-180, 1->180,
  //     Latitude  0->-90,  1->90
  
  double s, t;
  double lonScale = 1.0 / (this->ImageLongitudeLatitudeExtent[1]
                             -this->ImageLongitudeLatitudeExtent[0]);
  double latScale = 1.0 / (this->ImageLongitudeLatitudeExtent[3]
                             -this->ImageLongitudeLatitudeExtent[2]);
  // ImageLongitudeLatitudeExtent[0] maps to 0.0
  // ImageLongitudeLatitudeExtent[1] maps to 1.0
  for (int i=0; i<numPts; i++) 
    {
    s = ((*longitudePtr++) - this->ImageLongitudeLatitudeExtent[0]) * lonScale;
    t = ((*latitudePtr++) - this->ImageLongitudeLatitudeExtent[2]) * latScale;
    
    *tcoordPtr++ = s;
    *tcoordPtr++ = t;
    }

  output->GetPointData()->SetTCoords(newTCoords);
  newTCoords->Delete();
}


void vtkGeoComputeTextureCoordinates::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ImageLongitudeLatitudeExtent: " 
     << this->ImageLongitudeLatitudeExtent[0] 
     << this->ImageLongitudeLatitudeExtent[1]
     << this->ImageLongitudeLatitudeExtent[2] 
     << this->ImageLongitudeLatitudeExtent[3] << endl;
}
