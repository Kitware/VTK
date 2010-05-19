/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLineSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLineSource.h"

#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"

#include <math.h>
vtkStandardNewMacro(vtkLineSource);

vtkLineSource::vtkLineSource(int res)
{
  this->Point1[0] = -0.5;
  this->Point1[1] =  0.0;
  this->Point1[2] =  0.0;

  this->Point2[0] =  0.5;
  this->Point2[1] =  0.0;
  this->Point2[2] =  0.0;

  this->Resolution = (res < 1 ? 1 : res);

  this->SetNumberOfInputPorts(0);
}

int vtkLineSource::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),
               -1);
  return 1;
}

int vtkLineSource::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the ouptut
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  int numLines=this->Resolution;
  int numPts=this->Resolution+1;
  double x[3], tc[3], v[3];
  int i, j;
  vtkPoints *newPoints; 
  vtkFloatArray *newTCoords; 
  vtkCellArray *newLines;
  
  if (outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()) > 0)
    {
    return 1;
    }

  newPoints = vtkPoints::New();
  newPoints->Allocate(numPts);
  newTCoords = vtkFloatArray::New();
  newTCoords->SetNumberOfComponents(2);
  newTCoords->Allocate(2*numPts);
  newTCoords->SetName("Texture Coordinates");
  newLines = vtkCellArray::New();
  newLines->Allocate(newLines->EstimateSize(numLines,2));
//
// Generate points and texture coordinates
//
  for (i=0; i<3; i++)
    {
    v[i] = this->Point2[i] - this->Point1[i];
    }

  tc[1] = 0.0;
  tc[2] = 0.0;
  for (i=0; i<numPts; i++) 
    {
    tc[0] = ((double)i/this->Resolution);
    for (j=0; j<3; j++)
      {
      x[j] = this->Point1[j] + tc[0]*v[j];
      }
    newPoints->InsertPoint(i,x);
    newTCoords->InsertTuple(i,tc);
    }
//
//  Generate lines
//
  newLines->InsertNextCell(numPts);
  for (i=0; i < numPts; i++) 
    {
    newLines->InsertCellPoint (i);
    }
//
// Update ourselves and release memory
//
  output->SetPoints(newPoints);
  newPoints->Delete();

  output->GetPointData()->SetTCoords(newTCoords);
  newTCoords->Delete();

  output->SetLines(newLines);
  newLines->Delete();

  return 1;
}

void vtkLineSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Resolution: " << this->Resolution << "\n";

  os << indent << "Point 1: (" << this->Point1[0] << ", "
                               << this->Point1[1] << ", "
                               << this->Point1[2] << ")\n";

  os << indent << "Point 2: (" << this->Point2[0] << ", "
                               << this->Point2[1] << ", "
                               << this->Point2[2] << ")\n";
}
