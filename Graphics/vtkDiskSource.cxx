/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDiskSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDiskSource.h"

#include "vtkCellArray.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"

vtkStandardNewMacro(vtkDiskSource);

vtkDiskSource::vtkDiskSource()
{
  this->InnerRadius = 0.25;
  this->OuterRadius = 0.5;
  this->RadialResolution = 1;
  this->CircumferentialResolution = 6;

  this->SetNumberOfInputPorts(0);
}

int vtkDiskSource::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the ouptut
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType numPolys, numPts;
  double x[3];
  int i, j;
  vtkIdType pts[4];
  double theta, deltaRadius;
  double cosTheta, sinTheta;
  vtkPoints *newPoints; 
  vtkCellArray *newPolys;
  
  // Set things up; allocate memory
  //
  numPts = (this->RadialResolution + 1) * 
           (this->CircumferentialResolution + 1);
  numPolys = this->RadialResolution * this->CircumferentialResolution;
  newPoints = vtkPoints::New();
  newPoints->Allocate(numPts);
  newPolys = vtkCellArray::New();
  newPolys->Allocate(newPolys->EstimateSize(numPolys,4));

  // Create disk
  //
  theta = 2.0 * vtkMath::Pi() / this->CircumferentialResolution;
  deltaRadius = (this->OuterRadius - this->InnerRadius)/this->RadialResolution;

  for (i=0; i < this->CircumferentialResolution; i++) 
    {
    cosTheta = cos(i*theta);
    sinTheta = sin(i*theta);
    for (j=0; j <= this->RadialResolution; j++)
      {
      x[0] = (this->InnerRadius + j*deltaRadius) * cosTheta;
      x[1] = (this->InnerRadius + j*deltaRadius) * sinTheta;
      x[2] = 0.0;
      newPoints->InsertNextPoint(x);
      }
    }

  //  Create connectivity
  //
  for (i=0; i < this->CircumferentialResolution; i++) 
    {
    for (j=0; j < this->RadialResolution; j++) 
      {
      pts[0] = i*(this->RadialResolution+1) + j;
      pts[1] = pts[0] + 1;
      if ( i < (this->CircumferentialResolution-1) )
        {
        pts[2] = pts[1] + this->RadialResolution + 1;
        }
      else
        {
        pts[2] = j + 1;
        }
      pts[3] = pts[2] - 1;
      newPolys->InsertNextCell(4,pts);
      }
    }

  // Update ourselves and release memory
  //
  output->SetPoints(newPoints);
  newPoints->Delete();

  output->SetPolys(newPolys);
  newPolys->Delete();

  return 1;
}

void vtkDiskSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "InnerRadius: " << this->InnerRadius << "\n";
  os << indent << "OuterRadius: " << this->OuterRadius << "\n";
  os << indent << "RadialResolution: " << this->RadialResolution << "\n";
  os << indent << "CircumferentialResolution: " << this->CircumferentialResolution << "\n";
}
