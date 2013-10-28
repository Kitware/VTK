/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRegularPolygonSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRegularPolygonSource.h"

#include "vtkCellArray.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"

vtkStandardNewMacro(vtkRegularPolygonSource);

vtkRegularPolygonSource::vtkRegularPolygonSource()
{
  this->NumberOfSides = 6;
  this->Center[0] = 0.0; this->Center[1] = 0.0; this->Center[2] = 0.0;
  this->Normal[0] = 0.0; this->Normal[1] = 0.0; this->Normal[2] = 1.0;
  this->Radius = 0.5;
  this->GeneratePolygon = 1;
  this->GeneratePolyline = 1;
  this->OutputPointsPrecision = SINGLE_PRECISION;

  this->SetNumberOfInputPorts(0);
}

int vtkRegularPolygonSource::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),-1);

  return 1;
}

int vtkRegularPolygonSource::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // Get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // Get the output
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // We only produce one piece
  if (outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()) > 0)
    {
    return 1;
    }

  double x[3], r[3];
  int i, j, numPts=this->NumberOfSides;
  vtkPoints *newPoints;
  vtkCellArray *newPoly;
  vtkCellArray *newLine;

  // Prepare to produce the output; create the connectivity array(s)
  newPoints = vtkPoints::New();

  // Set the desired precision for the points in the output.
  if(this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
    {
    newPoints->SetDataType(VTK_DOUBLE);
    }
  else
    {
    newPoints->SetDataType(VTK_FLOAT);
    }

  newPoints->Allocate(numPts);

  if ( this->GeneratePolyline )
    {
    newLine = vtkCellArray::New();
    newLine->Allocate(newLine->EstimateSize(1,numPts));
    newLine->InsertNextCell(numPts+1);
    for (i=0; i<numPts; i++)
      {
      newLine->InsertCellPoint(i);
      }
    newLine->InsertCellPoint(0); //close the polyline
    output->SetLines(newLine);
    newLine->Delete();
    }

  if ( this->GeneratePolygon )
    {
    newPoly = vtkCellArray::New();
    newPoly->Allocate(newPoly->EstimateSize(1,numPts));
    newPoly->InsertNextCell(numPts);
    for (i=0; i<numPts; i++)
      {
      newPoly->InsertCellPoint(i);
      }
    output->SetPolys(newPoly);
    newPoly->Delete();
    }

  // Produce a unit vector in the plane of the polygon (i.e., perpendicular
  // to the normal)
  double n[3], axis[3], px[3], py[3];

  // Make sure the polygon normal is a unit vector
  n[0] = this->Normal[0];
  n[1] = this->Normal[1];
  n[2] = this->Normal[2];
  if ( vtkMath::Normalize(n) == 0.0 )
    {
    n[0] = 0.0;
    n[1] = 0.0;
    n[2] = 1.0;
    }

  // Cross with unit axis vectors and eventually find vector in the polygon plane
  int foundPlaneVector = 0;
  axis[0] = 1.0;
  axis[1] = 0.0;
  axis[2] = 0.0;
  vtkMath::Cross(n,axis,px);
  if ( vtkMath::Normalize(px) > 1.0E-3 )
    {
    foundPlaneVector = 1;
    }
  if ( ! foundPlaneVector )
    {
    axis[0] = 0.0;
    axis[1] = 1.0;
    axis[2] = 0.0;
    vtkMath::Cross(n,axis,px);
    if ( vtkMath::Normalize(px) > 1.0E-3 )
      {
      foundPlaneVector = 1;
      }
    }
  if ( ! foundPlaneVector )
    {
    axis[0] = 0.0;
    axis[1] = 0.0;
    axis[2] = 1.0;
    vtkMath::Cross(n,axis,px);
    vtkMath::Normalize(px);
    }
  vtkMath::Cross(px,n,py); //created two orthogonal axes in the polygon plane, px & py

  // Now run around normal vector to produce polygon points.
  double theta = 2.0 * vtkMath::Pi() / numPts;
  for (j=0; j<numPts; j++)
    {
    for (i=0; i<3; i++)
      {
      r[i] = px[i]*cos((double)j*theta) + py[i]*sin((double)j*theta);
      x[i] = this->Center[i] + this->Radius * r[i];
      }
    newPoints->InsertNextPoint(x);
    }

  output->SetPoints(newPoints);
  newPoints->Delete();

  return 1;
}

void vtkRegularPolygonSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Number of Sides: " << this->NumberOfSides << "\n";

  os << indent << "Center: (" << this->Center[0] << ", "
                              << this->Center[1] << ", "
                              << this->Center[2] << ")\n";

  os << indent << "Normal: (" << this->Normal[0] << ", "
                              << this->Normal[1] << ", "
                              << this->Normal[2] << ")\n";

  os << indent << "Radius: " << this->Radius << "\n";

  os << indent << "Generate Polygon: " << (this->GeneratePolygon ? "On\n" : "Off\n");

  os << indent << "Generate Polyline: " << (this->GeneratePolyline ? "On\n" : "Off\n");

  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
}
